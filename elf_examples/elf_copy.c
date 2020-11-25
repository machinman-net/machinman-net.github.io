/*
 * Copy an elf to an ELF object.
 * This is a demo program for libraries libelf and gelf. The resulting elf file
 * may not be loadable due to offset changes.
 * 2007.11.10 19:52:23 Emmanuel AZENCOT
 *   Creation
 * Sun Nov 22 20:33:20 CET 2009
 *   bug fix in error message \%s\"
 * 
 * build : gcc -Wall -g -o elf_copy -lelf elf_copy.c
 * TODO
 *  output a working binary
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

#define errx(code, fmt, args...) { \
   fprintf(stderr,#code ": error in file %s at line %d in function %s:", __FILE__,__LINE__,__FUNCTION__); \
   fprintf(stderr,fmt "\n", ##args); \
   exit(1); \
}
#define merrx(code, fmt, args...) { \
   fprintf(stderr,#code ": error in file %s at line %d in function %s:", __FILE__,__LINE__,__FUNCTION__); \
   fprintf(stderr,fmt "\n", ##args); \
   return 0; \
}

int melf_cpy_scn (Elf *elf_out, Elf_Scn  *scn_out, Elf *elf_in, Elf_Scn *scn_in) {
   Elf_Data *data_in, *data_out;
   GElf_Shdr shdr_in, shdr_out;
   size_t n;

   if (gelf_getshdr(scn_in, &shdr_in) != &shdr_in)
      merrx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));

   data_in = NULL; n = 0;
   while (n < shdr_in.sh_size && (data_in = elf_getdata(scn_in, data_in)) != NULL) {
      if ((data_out = elf_newdata(scn_out)) == NULL)
         merrx(EX_SOFTWARE, "elf_newdata() failed: %s.", elf_errmsg(-1));
      *data_out = *data_in;
   }

   if ( gelf_getshdr(scn_out, &shdr_out) != &shdr_out)
      merrx(EX_SOFTWARE, "gelf_getshdr() failed: %s.", elf_errmsg(-1));

   shdr_out = shdr_in;

   if ( gelf_update_shdr (scn_out, &shdr_out) == 0)
      merrx(EX_SOFTWARE, "gelf_update_shdr() failed: %s.", elf_errmsg(-1));

   return 1;
}
int main(int argc, char *argv[]) {
   int fd_in = -1, fd_out = -1;
   Elf *elf_in = 0, *elf_out = 0;

   Elf_Scn *scn_in, *scn_out;
   GElf_Ehdr ehdr_in, ehdr_out;

   size_t shstrndx;

   int scn_index = 0;

   if (argc != 3)
      errx(EX_USAGE, "USAGE: %s in-file-name out-file-name", argv[0]);

   if (elf_version(EV_CURRENT) == EV_NONE)
      errx(EX_SOFTWARE, "ELF library initialization failed: %s", elf_errmsg(-1));

   if ((fd_in = open(argv[1], O_RDONLY, 0)) < 0)
      err(EX_NOINPUT, "open \"%s\" failed", argv[1]);

   if ((elf_in = elf_begin(fd_in, ELF_C_READ, NULL)) == NULL)
      errx(EX_SOFTWARE, "elf_begin() failed: %s.", elf_errmsg(-1));

   if (gelf_getehdr(elf_in, &ehdr_in) != &ehdr_in)
      errx(EX_SOFTWARE, "gelf_getehdr() failed: %s.", elf_errmsg(-1));

   /* get string section index for section names */ 
   if (elf_getshstrndx(elf_in, &shstrndx) != 0)
       errx(EX_SOFTWARE, "getshstrndx() failed: %s, %d.", elf_errmsg(-1), shstrndx);

   /* Checks and warns */
   if ( elf_kind(elf_in) != ELF_K_ELF ) {
      fprintf(stderr, "%s : %s must be an ELF file.\n", argv[0], argv[1]);
      exit(1);
   }

   printf("Elf %s have %d sections\n", argv[1], ehdr_in.e_shnum);
   /* opent output elf */
   if ((fd_out = open(argv[2], O_WRONLY|O_CREAT, 0777)) < 0) 
      errx(EX_OSERR, "open \"%s\" failed", argv[2]);

   if ((elf_out = elf_begin(fd_out, ELF_C_WRITE, NULL)) == NULL)
      errx(EX_SOFTWARE, "elf_begin() failed: %s.", elf_errmsg(-1));

   /* create new elf header */
   if ( gelf_newehdr(elf_out, ehdr_in.e_ident[EI_CLASS]) == 0)
      errx(EX_SOFTWARE, "gelf_newehdr() failed: %s.", elf_errmsg(-1));
   if ( gelf_getehdr(elf_out, &ehdr_out) != &ehdr_out)
      errx(EX_SOFTWARE, "gelf_getehdr() failed: %s.", elf_errmsg(-1));
   ehdr_out = ehdr_in;
   ehdr_out.e_ehsize = 0;
   ehdr_out.e_phentsize = 0;
   ehdr_out.e_phnum = 0;
   ehdr_out.e_shentsize = 0;
   ehdr_out.e_shnum = 0;
   if ( gelf_update_ehdr (elf_out, &ehdr_out) == 0)
      errx(EX_SOFTWARE, "gelf_update_ehdr() failed: %s.", elf_errmsg(-1));

   {
      GElf_Phdr phdr_in, phdr_out;
      int ph_ndx;

      if ( ehdr_in.e_phnum && gelf_newphdr (elf_out, ehdr_in.e_phnum) == 0 ) 
         errx(EX_SOFTWARE, "gelf_newphdr() failed: %s.", elf_errmsg(-1));

      for ( ph_ndx = 0; ph_ndx < ehdr_in.e_phnum; ++ph_ndx ) {
         printf("Copying prog header %d\n", ph_ndx);

         if ( gelf_getphdr (elf_in, ph_ndx, &phdr_in) != &phdr_in )
            errx(EX_SOFTWARE, "gelf_getphdr() failed: %s.", elf_errmsg(-1));

         if ( gelf_getphdr (elf_out, ph_ndx, &phdr_out) != &phdr_out )
            errx(EX_SOFTWARE, "gelf_getphdr() failed: %s.", elf_errmsg(-1));
         phdr_out = phdr_in;

         if ( gelf_update_phdr (elf_out, ph_ndx, &phdr_out) == 0) 
            errx(EX_SOFTWARE, "gelf_update_phdr() failed: %s.", elf_errmsg(-1));
      }
   }
   /* copy sections to new elf */
   for ( scn_index = 1; scn_index < ehdr_in.e_shnum; scn_index++ ) {
      if ( (scn_in = elf_getscn(elf_in, scn_index)) == NULL) 
         errx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
      if ((scn_out = elf_newscn(elf_out)) == NULL)
         errx(EX_SOFTWARE, "elf_newscn() failed: %s.", elf_errmsg(-1));
      if ( melf_cpy_scn (elf_out, scn_out, elf_in, scn_in) == 0 ) 
         errx(EX_SOFTWARE, "melf_cpy_scn() failed: %s.", elf_errmsg(-1));
   }
   /* Adjust section offset in Phrs */
   {
      GElf_Phdr phdr_in, phdr_out;
      int ph_ndx;

      if ( ehdr_in.e_phnum && gelf_newphdr (elf_out, ehdr_in.e_phnum) == 0 ) 
         errx(EX_SOFTWARE, "gelf_newphdr() failed: %s.", elf_errmsg(-1));

      for ( ph_ndx = 0; ph_ndx < ehdr_in.e_phnum; ++ph_ndx ) {
         GElf_Shdr shdr_out;
         if ( gelf_getphdr (elf_in, ph_ndx, &phdr_in) != &phdr_in )
            errx(EX_SOFTWARE, "gelf_getphdr() failed: %s.", elf_errmsg(-1));

         if ( gelf_getphdr (elf_out, ph_ndx, &phdr_out) != &phdr_out )
            errx(EX_SOFTWARE, "gelf_getphdr() failed: %s.", elf_errmsg(-1));

         if ( !phdr_in.p_offset || phdr_in.p_type == PT_PHDR ) continue;
         /* Get section offset from input phdr */
         if ( (scn_in = gelf_offscn (elf_in, phdr_in.p_offset )) == NULL) 
            errx(EX_SOFTWARE, "gelf_offscn() failed: %s.", elf_errmsg(-1));
         /* Convert to section index */
         if ( (scn_index = elf_ndxscn (scn_in)) == 0)
            fprintf(stderr, "elf_ndxscn() failed for phdr %d : %s.", ph_ndx, elf_errmsg(-1));
         /* get out section at same index from out elf */
         if ( (scn_out   = elf_getscn(elf_out, scn_index)) == NULL) 
            errx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
         /* get out section offset from section header */
         if ( gelf_getshdr(scn_out, &shdr_out) != &shdr_out)
            merrx(EX_SOFTWARE, "gelf_getshdr() failed: %s.", elf_errmsg(-1));

         phdr_out.p_offset = shdr_out.sh_offset;

         if ( gelf_update_phdr (elf_out, ph_ndx, &phdr_out) == 0) 
            errx(EX_SOFTWARE, "gelf_update_phdr() failed: %s.", elf_errmsg(-1));
      }
   }
   if (elf_update(elf_out, ELF_C_WRITE) < 0)
      errx(EX_SOFTWARE, "elf_update() failed: %s.", elf_errmsg(-1));

   elf_end(elf_out);
   close(fd_out);
   elf_end(elf_in);
   close(fd_in);

   exit(EX_OK);
}
 
