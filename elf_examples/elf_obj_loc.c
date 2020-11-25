/*
 * Copyright (C) 2007 Emmanuel AZENCOT All Rights Reserved. * 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * The address of the Free Software Foundation is
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA 02110-1301, USA.
 */

/*
 * Prepare an elf relocatable object for disassemby (objdump)
 *
 * 2007-11-14 16:11:49 Emmanuel AZENCOT
 *   Creation
 * 2008-01-21 15:44:08
 *   Fix resolution for data
 * Sun Nov 22 20:33:20 CET 2009
 *   bug fix in error message \%s\"
 *
 * build : gcc -Wall -g -o elf_obj_loc -lelf elf_obj_loc.c
 * TODO :
 *  clean output, have a verbose mode
 *  use info file to retreive .rel corresponding section (instead of name)
 *  test big endian object
 *  test 64 bits object
 *  test other processors
 *  deal with other ABI
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <err.h>
#include <gelf.h>

#include </usr/include/asm/byteorder.h>

#define endian_to_target(tgt_is_be, value) ( (sizeof(value) == 4)?\
      ((tgt_is_be)?__cpu_to_be32(value):__cpu_to_le32(value)):        \
      ((tgt_is_be)?__cpu_to_be64(value):__cpu_to_le64(value)) )


#define errx(code, fmt, args...) { \
   fprintf(stderr,#code ": error in file %s at line %d in function %s:", __FILE__,__LINE__,__FUNCTION__); \
   fprintf(stderr,fmt "\n", ##args); \
   exit(1); \
}
#define merrx(code, fmt, args...) { \
   fprintf(stderr,#code ": error in file %s at line %d in function %s:", __FILE__,__LINE__,__FUNCTION__); \
   fprintf(stderr,fmt "\n", ##args); \
   return -1; \
}
/**
 * @memo Find a section given its name
 * @doc
 *   Look in evry secttion until it find a section name "scn_name"
 * @param __elf Elf Handle to search in
 * @param scn_name section name
 * @return -1 error, 0 not found, section index
 */
int melf_scn_by_name(Elf *__elf, char *scn_name) {
   size_t shstrndx;
   size_t scn_ndx = 0;
   Elf_Scn *scn;
   GElf_Shdr shdr;
   char *name;

   if (elf_getshstrndx(__elf, &shstrndx) != 0)
      merrx(EX_SOFTWARE, "getshstrndx() failed: %s.", elf_errmsg(-1));

   while ((scn = elf_getscn(__elf, scn_ndx)) != NULL) {

      if (gelf_getshdr(scn, &shdr) != &shdr)
         merrx(EX_SOFTWARE, "gelf_getshdr() failed: %s.", elf_errmsg(-1));

      if ((name = elf_strptr(__elf, shstrndx, shdr.sh_name)) == NULL)
         merrx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));
      if ( !strcmp(scn_name, name) ) 
         return scn_ndx;
      scn_ndx++;
   }
   return 0;
}
/** 
 * @memo Symbol table section adtatation
 * @doc 
 *   As symbol table contains section indexes, there must be changed according to new elf
 * section layout
 * @param __elf a valid input/output elf handle
 * @param conv_tbl a section table conversion (see melf_rel_scn_layout)
 * @param len conv_tbl number of entries
 * @return -1 error, symbol table section index
 */
int melf_scn_sym_shconv(Elf *__elf, ssize_t *conv_tbl, size_t len) {
   ssize_t scn_ndx_sym;
   Elf_Scn *scn_sym;
   Elf_Data *data_sym;
   size_t ndx_sym;
   GElf_Sym sym;

   if ( (scn_ndx_sym = melf_scn_by_name(__elf, ".symtab")) <= 0) 
      merrx(EX_SOFTWARE, "melf_scn_by_name() failed to find section .symtab.");

   if ( (scn_sym = elf_getscn(__elf, scn_ndx_sym)) == NULL) 
      merrx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));

   if ( (data_sym = elf_getdata(scn_sym, 0)) == NULL )
      merrx(EX_SOFTWARE, "elf_getdata() failed: %s.", elf_errmsg(-1));

   ndx_sym = 0;
   while ( gelf_getsym (data_sym, ndx_sym, &sym) == &sym ) {
      // if ( sym.st_shndx > SHN_LORESERVE && ++ndx_sym) continue;
      if ( 0 < (ssize_t) sym.st_shndx && (ssize_t) sym.st_shndx < len) {
         if ( conv_tbl[sym.st_shndx] == -1 )
            merrx(EX_SOFTWARE, "Can't convert section (0x%x) for symbol index %d", sym.st_shndx, ndx_sym);
         sym.st_shndx = conv_tbl[sym.st_shndx];
         if ( gelf_update_sym (data_sym, ndx_sym, &sym) == 0)
            merrx(EX_SOFTWARE, "gelf_update_sym() failed: %s.", elf_errmsg(-1));
      }
      ++ndx_sym;
   }
   if ( elf_flagdata (data_sym, ELF_C_SET, ELF_F_DIRTY) == 0)
      merrx(EX_SOFTWARE, "elf_flagdata() failed: %s.", elf_errmsg(-1));

   return scn_ndx_sym;
}
/**
 * @memo Section content Hexa dump 
 * @ doc
 *  Output section content in hexadecimal format.
 *  Section can be given by handle or by name.
 * @param __elf a valid input elf handle
 * @param scn a valid scn handle or 0
 * @param scn_name a section name or 0
 * @return -1 error, 0 okay
 */
int melf_scn_dump(Elf *__elf, Elf_Scn *scn, char *scn_name) {
   GElf_Shdr shdr;
   Elf_Data *data;
   size_t shstrndx;
   char *p; int i;

   if ( !scn ) {
      size_t scn_ndx;
      if ( !scn_name ) 
         merrx(EX_SOFTWARE, "Both name and scn handle are nul");
      if ( (scn_ndx = melf_scn_by_name(__elf, scn_name)) <= 0) 
         merrx(EX_SOFTWARE, "melf_scn_by_name() failed to find section %s.", scn_name);
      if ( (scn = elf_getscn(__elf, scn_ndx)) == NULL) 
         merrx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));

   }
   if (elf_getshstrndx(__elf, &shstrndx) != 0)
      merrx(EX_SOFTWARE, "getshstrndx() failed: %s.", elf_errmsg(-1));

   if (gelf_getshdr(scn, &shdr) != &shdr)
      merrx(EX_SOFTWARE, "getshdr(shstrndx) failed: %s.", elf_errmsg(-1));
   if ((scn_name = elf_strptr(__elf, shstrndx, shdr.sh_name)) == NULL)
      merrx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));
   if ( (data = elf_getdata(scn, 0)) == NULL )
      merrx(EX_SOFTWARE, "elf_getdata() failed: %s.", elf_errmsg(-1));

   printf("Dump section %s: size=%jd\n", scn_name, (uintmax_t) shdr.sh_size);

   p = (char *) data->d_buf;
   for ( i = 0; i < data->d_size; i++ ) {
      printf("%02hhx ", *p++);
      if ( !((i + 17) %16) ) printf("\n");
   }
   printf("\n");
   return 0;
}
/** 
 * @memo Symbol table listing
 * @doc
 *   print the symbol table. Symbol strings are taken from the section named
 * ".strtab". (Usualy, the strings and taken from section index pointed by
 * sh_link field of section header).
 * @param __elf valid input elf handle
 * @return -1 error, symbol table section index
 */
int melf_scn_sym_list(Elf *__elf) {
   ssize_t scn_ndx_sym, scn_ndx_str;
   Elf_Scn *scn_sym;
   Elf_Data *data_sym;
   GElf_Sym sym;
   size_t ndx_sym;

   if ( (scn_ndx_sym = melf_scn_by_name(__elf, ".symtab")) <= 0) 
      merrx(EX_SOFTWARE, "melf_scn_by_name() failed to find section .symtab.");
   /* should use shdr_sym.sh_link ? */
   if ( (scn_ndx_str = melf_scn_by_name(__elf, ".strtab")) <= 0) 
      merrx(EX_SOFTWARE, "melf_scn_by_name() failed to find section .symtab.");

   if ( (scn_sym = elf_getscn(__elf, scn_ndx_sym)) == NULL) 
      merrx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));

   if ( (data_sym = elf_getdata(scn_sym, 0)) == NULL )
      merrx(EX_SOFTWARE, "elf_getdata() failed: %s.", elf_errmsg(-1));

   /* walk throught symbols table */
   printf("name   value   size   info   other shndx\n");
   ndx_sym = 0;
   while ( gelf_getsym (data_sym, ndx_sym, &sym) == &sym ) {
      char *name;
      if ((name = elf_strptr(__elf, scn_ndx_str, sym.st_name)) == NULL)
         merrx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));

      printf("%05x %08llx %04llx %08x %05x %04x %s\n",
             sym.st_name, sym.st_value, sym.st_size, sym.st_info, sym.st_other, sym.st_shndx, name);
      ++ndx_sym;
   }

   return scn_ndx_sym;
}

/**
 * @memo Symbol table locatioon adtatation
 * @doc
 *   Produce a trivial memory mapping for the alloc sections. Change sections adresses
 * according to it.
 *   Give all UNDEF symbol a recognisable location and change value in symbol table.
 * @param __elf a valid input/output elf handle
 * @return -1 error, symbol table section index
 */
int melf_scn_sym_lnk(Elf *__elf) {
   ssize_t scn_ndx_sym, scn_ndx;
   Elf_Scn *scn_sym, *scn;
   GElf_Shdr shdr;
   Elf_Data *data_sym;
   Elf64_Addr PC = 0x08048000;
   size_t ndx_sym;
   GElf_Sym sym;


   if ( (scn_ndx_sym = melf_scn_by_name(__elf, ".symtab")) <= 0) 
      merrx(EX_SOFTWARE, "melf_scn_by_name() failed to find section .symtab.");
   if ( (scn_sym = elf_getscn(__elf, scn_ndx_sym)) == NULL) 
      merrx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));

   if ( (data_sym = elf_getdata(scn_sym, 0)) == NULL )
      merrx(EX_SOFTWARE, "elf_getdata() failed: %s.", elf_errmsg(-1));

   /* Map alloc sectionq in memory */
   for ( scn = 0; (scn = elf_nextscn(__elf, scn)) != NULL; ) {
      scn_ndx = elf_ndxscn (scn);

      if (gelf_getshdr(scn, &shdr) != &shdr)
         merrx(EX_SOFTWARE, "gelf_getshdr() failed: %s.", elf_errmsg(-1));
      if ( !(shdr.sh_flags & SHF_ALLOC) && ++scn_ndx ) 
         continue;

      shdr.sh_addr = PC;
      if ( shdr.sh_addralign ) 
         PC += ((shdr.sh_size + shdr.sh_addralign-1)/shdr.sh_addralign)*shdr.sh_addralign;
      else 
         PC += shdr.sh_size;
      if ( elf_flagshdr (scn, ELF_C_SET, ELF_F_DIRTY) == 0)
         merrx(EX_SOFTWARE, "elf_flagshdr() failed: %s.", elf_errmsg(-1));
      if ( gelf_update_shdr (scn, &shdr) == 0)
         merrx(EX_SOFTWARE, "gelf_update_shdr() failed: %s.", elf_errmsg(-1));
   }
   /* give UNDEF a decent location */
   PC = 0x02500000;
   ndx_sym = 0;
   while ( gelf_getsym (data_sym, ndx_sym, &sym) == &sym ) {
      if ( sym.st_shndx != 0 && ++ndx_sym) continue;
      sym.st_value = PC;
      sym.st_shndx = SHN_ABS;
      PC += 0x10;
      if ( gelf_update_sym (data_sym, ndx_sym, &sym) == 0)
         merrx(EX_SOFTWARE, "gelf_update_sym() failed: %s.", elf_errmsg(-1));

      ++ndx_sym;
   }
   if ( elf_flagdata (data_sym, ELF_C_SET, ELF_F_DIRTY) == 0)
      merrx(EX_SOFTWARE, "elf_flagdata() failed: %s.", elf_errmsg(-1));

   return scn_ndx_sym;
}
/**
 * @memo target data encoding 
 * 0 little endian
 * 1 big endian
 */
int target_is_big_endian;
/** 
 * @memo Relocate section
 * @doc
 *   Apply the relocations contained in .rel section to a section.
 * Note : Symbole table section .symtab must have adapted prior.
 * @param elf Elf handler for section beeing relocated
 * @param scn_ndx_out Section index to relocate
 * @param scn_ndx_rel Reloc section index 
 * @param scn_ndx_sym Symbol table section 
 * @return -1 on error, 0 else
 */
int melf_scn_rel(Elf *elf_out, size_t scn_ndx_out, 
                 Elf *elf_rel, size_t scn_ndx_rel, 
                 Elf *elf_sym, size_t scn_ndx_sym) {
   Elf_Scn *scn_out, *scn_rel, *scn_sym;
   GElf_Shdr shdr_out, shdr_rel, shdr_sym;
   Elf_Data *data_out, *data_rel, *data_sym;
   int rel_i;

   printf("index out : %d, rel : %d, sym : %d", scn_ndx_out, scn_ndx_rel, scn_ndx_sym);

   if ( (scn_rel = elf_getscn(elf_rel, scn_ndx_rel)) == NULL) 
      merrx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));
   if ( (scn_out = elf_getscn(elf_out, scn_ndx_out)) == NULL) 
      merrx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));
   if ( (scn_sym = elf_getscn(elf_sym, scn_ndx_sym)) == NULL) 
      merrx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));

   if (gelf_getshdr(scn_rel, &shdr_rel) != &shdr_rel)
      merrx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
   if (gelf_getshdr(scn_out, &shdr_out) != &shdr_out)
      merrx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
   if (gelf_getshdr(scn_sym, &shdr_sym) != &shdr_sym)
      merrx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));

   if ( (data_out = elf_getdata(scn_out, 0)) == NULL)
      merrx(EX_SOFTWARE, "getdata() failed: %s.", elf_errmsg(-1));
   if ( shdr_out.sh_size != data_out->d_size ) 
      fprintf(stderr, "warning tgt data containt more than one chunk\n");

   if ( (data_rel = elf_getdata(scn_rel, 0)) == NULL)
      merrx(EX_SOFTWARE, "getdata() failed: %s.", elf_errmsg(-1));
   if ( shdr_rel.sh_size != data_rel->d_size ) 
      fprintf(stderr, "warning rel data is in more than one chunk\n");

   if ( (data_sym = elf_getdata(scn_sym, 0)) == NULL)
      merrx(EX_SOFTWARE, "getdata() failed: %s.", elf_errmsg(-1));
   if ( shdr_sym.sh_size != data_sym->d_size ) 
      fprintf(stderr, "warning tgt data containt more than one chunk\n");

   for (rel_i = 0; ; rel_i++ ) {
      GElf_Rel rel;
      GElf_Sym sym;
      Elf_Scn *scn_dst;
      GElf_Shdr shdr_dst;
      Elf64_Addr shaddr = 0;
     
      if ( gelf_getrel (data_rel, rel_i, &rel) != &rel ) 
         break;
      
      if ( gelf_getsym (data_sym, GELF_R_SYM(rel.r_info), &sym) != &sym )
         merrx(EX_SOFTWARE, "gelf_getsym() failed: %s. Symbol %lld not found", elf_errmsg(-1), GELF_R_SYM(rel.r_info));

      if ( sym.st_shndx < SHN_LORESERVE ) {
         if ( (scn_dst = elf_getscn(elf_out, sym.st_shndx)) == NULL) 
            merrx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));
         if (gelf_getshdr(scn_dst, &shdr_dst) != &shdr_dst)
            merrx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
         shaddr = shdr_dst.sh_addr;
      }
      switch (GELF_R_TYPE(rel.r_info)) {
      case R_386_32:
         *((uint32_t *)(((char *)data_out->d_buf) + rel.r_offset)) += 
                          endian_to_target(target_is_big_endian,(uint32_t)(sym.st_value +  shaddr));
         break;
      case R_386_PC32: {

         *((uint32_t *)(((char *)data_out->d_buf) + rel.r_offset)) = 
                          endian_to_target(target_is_big_endian,(uint32_t)(sym.st_value + shaddr - (shdr_out.sh_addr + rel.r_offset) - 4));
         break;
      }
      default:
         merrx(EX_SOFTWARE, "Can you help me to about this type of reloc (%lld) ? Oh I would do much you tacke care of this ... please ...\n", GELF_R_TYPE(rel.r_info));
      }
   }
   if ( elf_flagdata (data_out, ELF_C_SET, ELF_F_DIRTY) == 0)
      merrx(EX_SOFTWARE, "elf_flagdata() failed: %s.", elf_errmsg(-1));

   if ( gelf_update_shdr (scn_out, &shdr_out) == 0)
      errx(EX_SOFTWARE, "gelf_update_shdr() failed: %s.", elf_errmsg(-1));
   // printf("@%d ELF error %s\n", __LINE__, elf_errmsg(-1));
   return 0;
}
/**
 * @memo Build output section table
 * @doc
 *   The table is indexed by input section indexes. For each of them, the value
 * can be (-1) not used or the output section.
 * @param __elf valid input elf handle
 * @param conv_tbl an array of at least the number of section in __elf
 * @return -1 on error, number of sections in output.
 */
int melf_rel_scn_layout(Elf *__elf, ssize_t *conv_tbl) {
   GElf_Ehdr ehdr;
   GElf_Shdr shdr;
   Elf_Scn *scn;
   size_t ndx_out = 1;

   if (gelf_getehdr(__elf, &ehdr) != &ehdr)
      errx(EX_SOFTWARE, "gelf_getehdr() failed: %s.", elf_errmsg(-1));

   conv_tbl[0] = 0;
   /* set allocable sections first */
   for ( scn = 0; (scn = elf_nextscn(__elf, scn)) != NULL; ) {
      conv_tbl[elf_ndxscn (scn)] = -1;
      if (gelf_getshdr(scn, &shdr) != &shdr)
         merrx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
      if ( shdr.sh_type == SHT_REL || shdr.sh_type == SHT_RELA) continue;
      if ( !(shdr.sh_flags & SHF_ALLOC) ) continue;
      conv_tbl[elf_ndxscn (scn)] = ndx_out++;
   }
   for ( scn = 0; (scn = elf_nextscn(__elf, scn)) != NULL; ) {
      if (gelf_getshdr(scn, &shdr) != &shdr)
         merrx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
      if ( shdr.sh_type == SHT_REL || shdr.sh_type == SHT_RELA) continue;
      if ( (shdr.sh_flags & SHF_ALLOC) ) continue;
      conv_tbl[elf_ndxscn (scn)] = ndx_out++;
   }
   for ( scn = 0; (scn = elf_nextscn(__elf, scn)) != NULL; ) {
      printf ( "section transfert table[%2d] = %2d\n", elf_ndxscn (scn), conv_tbl[elf_ndxscn (scn)]);
   }
   return ndx_out;
}
/** 
 * @memo Find .rel section correspondind to a section
 * @doc
 *   Get peer section .rel that correspond to a section. The criteria to find peer is 
 * done througth the section name. 
 *  Should use sh_link and sh_info instead.
 * @param elf Elf handler
 * @param scn_ndx Section index
 * @return -1 on error, 0 not find, or scn index
 */
int melf_find_rel_section(Elf *__elf, size_t scn_ndx) {
   size_t shstrndx;
   Elf_Scn *scn, *scn_r;
   GElf_Shdr shdr, shdr_r;
   char *scn_name, *scn_name_r;

   if (elf_getshstrndx(__elf, &shstrndx) != 0)
      merrx(EX_SOFTWARE, "getshstrndx() failed: %s.", elf_errmsg(-1));
   if ( (scn = elf_getscn(__elf, scn_ndx)) == NULL) 
      merrx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));
   if (gelf_getshdr(scn, &shdr) != &shdr)
      merrx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
   if ((scn_name = elf_strptr(__elf, shstrndx, shdr.sh_name)) == NULL)
      merrx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));

   for ( scn_r = 0; (scn_r = elf_nextscn(__elf, scn_r)) != NULL; ) {

      if ( gelf_getshdr(scn_r, &shdr_r) != &shdr_r)
         merrx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));

      if ( shdr_r.sh_type != SHT_REL && shdr_r.sh_type != SHT_RELA) continue;

      if ((scn_name_r = elf_strptr(__elf, shstrndx, shdr_r.sh_name)) == NULL)
         merrx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));
      if ( memcmp(scn_name_r, ".rel", 4) ) continue;
      if ( strcmp(scn_name_r + 4, scn_name) ) continue;
      return elf_ndxscn (scn_r);
   }

   return 0;
}
int melf_is_scn_reloc(Elf_Scn  *scn) {
   GElf_Shdr shdr;

   if (gelf_getshdr(scn, &shdr) != &shdr)
      errx(EX_SOFTWARE, "gelf_getshdr() failed: %s.", elf_errmsg(-1));

   if ( shdr.sh_type == SHT_REL || shdr.sh_type == SHT_RELA) {
      return 1;
   } else {
      return 0;
   }
   return -1;
}
/**
 * @memo Prepare an elf from relocatable object for disassemble
 * @doc
 *  binutils objdump disassemble does not give a correct output for relocatable
 * object, mainly because it does not apply relocation section indications to 
 * the corresponding section.
 * This tool handle this relocation.
 */
int main(int argc, char *argv[]) {
   int fd_in = -1, fd_out = -1;
   Elf *elf_in = 0, *elf_out = 0;

   Elf_Scn  *scn_in, *scn_out;
   Elf_Data *data_in, *data_out;
   GElf_Ehdr ehdr_in, ehdr_out;

   GElf_Shdr shdr_in, shdr_out;
   size_t n, shstrndx;

   ssize_t *v_scn_in_2_out;
   char **sh_names;

   int scn_ndx_sym, scn_ndx_out = 0, scn_ndx_in;

   if (argc != 3)
      errx(EX_USAGE, "USAGE: %s in-file-name out-file-name", argv[0]);

   if (elf_version(EV_CURRENT) == EV_NONE)
      errx(EX_SOFTWARE, "ELF library initialization failed: %s", elf_errmsg(-1));

   /* Elf input setup */
   if ((fd_in = open(argv[1], O_RDONLY, 0)) < 0)
      err(EX_NOINPUT, "open \"%s\" failed", argv[1]);

   if ((elf_in = elf_begin(fd_in, ELF_C_READ, NULL)) == NULL)
      errx(EX_SOFTWARE, "elf_begin() failed: %s.", elf_errmsg(-1));

   if (gelf_getehdr(elf_in, &ehdr_in) != &ehdr_in)
      errx(EX_SOFTWARE, "gelf_getehdr() failed: %s.", elf_errmsg(-1));

   /* Checks and warns */
   if ( elf_kind(elf_in) != ELF_K_ELF || ehdr_in.e_type != ET_REL) {
      fprintf(stderr, "%s : %s must be an elf relocatable object\n", argv[0], argv[1]);
      exit(1);
   }
   if ( ehdr_in.e_ident[EI_OSABI] != ELFOSABI_LINUX ) {
      fprintf(stderr, "%s warning : target OS not implemented\n", argv[0]);
   }
   switch ( ehdr_in.e_ident[EI_DATA] ) {
   case ELFDATA2LSB:
      target_is_big_endian = 0;
      break;
   case ELFDATA2MSB:
      fprintf(stderr, "%s warning : big endian data encoding is not tested\n", argv[0]);
      target_is_big_endian = 1;
      break;
   default :
      fprintf(stderr, "%s : unsupported data type (%d)\n", argv[0], ehdr_in.e_ident[EI_DATA]);
      break;
   }
   if ( ehdr_in.e_machine != EM_386 ) {
      fprintf(stderr, "%s warning : machine not implemented\n", argv[0]);
   }

   printf("ELF %s contains %d sections\n", argv[1], ehdr_in.e_shnum);

   if (!(sh_names = (char **)malloc(ehdr_in.e_shnum*sizeof(char *))))
      errx(EX_SOFTWARE, "malloc \"%s\" failed", strerror(errno));

   if (elf_getshstrndx(elf_in, &shstrndx) != 0)
       errx(EX_SOFTWARE, "getshstrndx() failed: %s, %d.", elf_errmsg(-1), shstrndx);

   for ( scn_ndx_in = 0; scn_ndx_in < ehdr_in.e_shnum; scn_ndx_in++ ) {
      if ((scn_in = elf_getscn(elf_in, scn_ndx_in)) == NULL)
         errx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));

      if (gelf_getshdr(scn_in, &shdr_in) != &shdr_in)
         errx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));

      if ((sh_names[scn_ndx_in] = elf_strptr(elf_in, shstrndx, shdr_in.sh_name)) == NULL)
         errx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));
   }

//   if ((fd_out = open(argv[2], O_WRONLY|O_CREAT, 0777)) < 0) 
   if ((fd_out = open(argv[2], O_RDWR|O_CREAT, 0777)) < 0) 
      errx(EX_OSERR, "open \"%s\" failed", argv[2]);

   if ((elf_out = elf_begin(fd_out, ELF_C_WRITE, NULL)) == NULL)
//   if ((elf_out = elf_begin(fd_out, ELF_C_RDWR, NULL)) == NULL)
      errx(EX_SOFTWARE, "elf_begin() failed: %s.", elf_errmsg(-1));

   /* Create and set new Elf header */
   if ( gelf_newehdr(elf_out, ehdr_in.e_ident[EI_CLASS]) == 0)
      errx(EX_SOFTWARE, "gelf_newehdr() failed: %s.", elf_errmsg(-1));

   if ( gelf_getehdr(elf_out, &ehdr_out) != &ehdr_out)
      errx(EX_SOFTWARE, "gelf_getehdr() failed: %s.", elf_errmsg(-1));
    ehdr_out = ehdr_in;
    if ( gelf_update_ehdr (elf_out, &ehdr_out) == 0)
      errx(EX_SOFTWARE, "gelf_update_ehdr() failed: %s.", elf_errmsg(-1));

   /* Build section layout table */
   if (!(v_scn_in_2_out = (ssize_t *)malloc(ehdr_in.e_shnum * sizeof(ssize_t))))
      errx(EX_SOFTWARE, "malloc \"%s\" failed", strerror(errno));
   v_scn_in_2_out[0] = 0;
   if ( (ssize_t)(scn_ndx_out = melf_rel_scn_layout(elf_in, v_scn_in_2_out)) < 0 )
      errx(EX_SOFTWARE, "melf_rel_scn_layout() failed.");
   ehdr_out.e_shnum = scn_ndx_out;

   /* copy all sections to new elf execpt rel.section followin v_scn_in2_out layout */
   printf("Copy section to new elf : ");
   for ( scn_ndx_out = 1; scn_ndx_out < ehdr_out.e_shnum; scn_ndx_out++ ) {

      /* get source section corresponding to scn_ndx_out in layout */
      for ( scn_ndx_in = 0; scn_ndx_in < ehdr_in.e_shnum; scn_ndx_in++) {
         if ( scn_ndx_out == v_scn_in_2_out[scn_ndx_in] ) goto found_scn_source;
      }
      continue;
      found_scn_source:

      /* Recognize and set section names string table index */
      if ( shstrndx == scn_ndx_in ) ehdr_out.e_shstrndx = scn_ndx_out;

      if ((scn_in = elf_getscn(elf_in, scn_ndx_in)) == NULL)
         errx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));

      /* copy section to new elf */
      if (gelf_getshdr(scn_in, &shdr_in) != &shdr_in)
         errx(EX_SOFTWARE, "gelf_getshdr() failed: %s.", elf_errmsg(-1));

      if ((scn_out = elf_newscn(elf_out)) == NULL)
         errx(EX_SOFTWARE, "elf_newscn() failed: %s.", elf_errmsg(-1));
      data_in = NULL; n = 0;
      while (n < shdr_in.sh_size && (data_in = elf_getdata(scn_in, data_in)) != NULL) {
         if ((data_out = elf_newdata(scn_out)) == NULL)
            errx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));
         *data_out = *data_in;
         n +=  data_in->d_size;
      }

      if ( gelf_getshdr(scn_out, &shdr_out) != &shdr_out)
         errx(EX_SOFTWARE, "gelf_getshdr() failed: %s.", elf_errmsg(-1));

      shdr_out = shdr_in;
      /* convert link to new section numbering */
      if ( shdr_in.sh_link )
         shdr_out.sh_link = v_scn_in_2_out[shdr_in.sh_link];
      
      if ( gelf_update_shdr (scn_out, &shdr_out) == 0)
         errx(EX_SOFTWARE, "gelf_update_shdr() failed: %s.", elf_errmsg(-1));

      printf("%d<%d ", scn_ndx_out, scn_ndx_in);
   }
   printf("done\n");

   /* update ehdr to record e_shstrndx */
    if ( gelf_update_ehdr (elf_out, &ehdr_out) == 0)
      errx(EX_SOFTWARE, "gelf_update_ehdr() failed: %s.", elf_errmsg(-1));

   if (elf_update(elf_out, ELF_C_WRITE) < 0)
      errx(EX_SOFTWARE, "elf_update() failed: %s.", elf_errmsg(-1));

   elf_end(elf_out);
   /* 
    * I did not found an other way to modify section in out elf 
    * other than to reopen it in read/write mode 
    */ 
   if ((elf_out = elf_begin(fd_out, ELF_C_RDWR, NULL)) == NULL)
      errx(EX_SOFTWARE, "elf_begin() failed: %s.", elf_errmsg(-1));

    if ( melf_scn_sym_shconv(elf_out, v_scn_in_2_out, ehdr_in.e_shnum) < 0)
      errx(EX_SOFTWARE, "melf_scn_sym_shconv failled");
   if (elf_update(elf_out, ELF_C_NULL) < 0)
      errx(EX_SOFTWARE, "elf_update() failed: %s.", elf_errmsg(-1));

   /* Adapt symbol table */
   if ( (scn_ndx_sym = melf_scn_sym_lnk(elf_out)) < 0) 
      errx(EX_SOFTWARE, "melf_scn_sym_lnk() failed");
   if (elf_update(elf_out, ELF_C_NULL) < 0)
      errx(EX_SOFTWARE, "elf_update() failed: %s.", elf_errmsg(-1));

//     if (melf_scn_sym_list(elf_out) < 0 ) exit(1);
//     if (melf_scn_dump(elf_out, 0, ".symtab") <0 ) exit(1);
//     if (melf_scn_dump(elf_out, 0, ".shstrtab") <0 ) exit(1);

   /* peer section and rel.section */
   for ( scn_ndx_out = 1; scn_ndx_out < ehdr_out.e_shnum; scn_ndx_out++ ) {
      ssize_t scn_rel_i;
      /* skip .rel<.sections> */
      for ( scn_ndx_in = 0; scn_ndx_in < ehdr_in.e_shnum; scn_ndx_in++) {
         if ( scn_ndx_out == v_scn_in_2_out[scn_ndx_in] ) goto found_scn_source_2;
      }
      continue;
      found_scn_source_2:

      /* track section count for section names string table index */
      printf("[%d] : %s", scn_ndx_in, sh_names[scn_ndx_in]);
      switch ( (scn_rel_i = melf_find_rel_section(elf_in, scn_ndx_in)) ) {
      case -1: errx(EX_SOFTWARE, "melf_find_rel_section() failed"); break;
      case  0: break;
      default : 
         /* Locate section if there is a .rel for it */
         printf(" linked with section : [%d]", scn_rel_i);
         if ( melf_scn_rel(elf_out, scn_ndx_out, elf_in, scn_rel_i, elf_out, scn_ndx_sym) < 0 ) 
            errx(EX_SOFTWARE, "melf_scn_reloc() failed");
      }          
      printf("\n");
   }

/*
   if (elf_update(elf_out, ELF_C_NULL) < 0)
      errx(EX_SOFTWARE, "elf_update(NULL) failed: %s.", elf_errmsg(-1));
*/
   if (elf_update(elf_out, ELF_C_WRITE) < 0)
      errx(EX_SOFTWARE, "elf_update() failed: %s.", elf_errmsg(-1));

   if ( elf_out ) elf_end(elf_out);
   if ( fd_out >= 0 ) close(fd_out);
   if ( elf_in ) elf_end(elf_in);
   if ( fd_in >= 0 ) close(fd_in);


   exit(EX_OK);
}
 
