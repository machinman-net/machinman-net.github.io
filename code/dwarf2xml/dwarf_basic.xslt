<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<!-- 
/* 
 * Copyright (C) 2010 by Emmanuel Azencot under the GNU GPL license 2.
 * You should have received a copy of the licence 
 * 
 *                          Version 2, June 1991
 *      Copyright (C) 1989, 1991 Free Software Foundation, Inc.
 *      675 Mass Ave, Cambridge, MA 02139, USA
 *      
 * GPL license along with this software os. if you did not you can find it
 * at http://www.gnu.org/.
 *
 * # 
 * # # project hosted at http://machinman.net/code/dwarf2xml
 * #
 */
 -->
<!-- TODO
Tue Jun 28 16:13:46 CEST 2016
Added debug_pubtypes, debug_weaknames, debug_varnames debug_funcnames debug_typenames

OK  macinfo : link fileno
OK  macinfo : separate table for each file ?
OK  info : <import>501, check if it is a ref to a die (501 - namespace : std). 
OK  info : merge decl_column with decl_line
OK : DW_AT_specification  "value is a reference to the debugging information entry"
OK : DW_AT_namelist_item attribute whose value is a reference to the debugging information entry
OK : DW_AT_friend attribute, whose value is a reference to the debugging information entry
OK : DW_AT_object_pointer attribute whose value is a reference to the formal parameter entry 
OK : DW_AT_containing_type attribute, whose value is a reference to a debugging information entry

./dwarf2xml -all test_dwarf  > toto.xml; xmllint ++valid toto.xml 2>&1 | tee toto.err | more
./dwarf2xml -all ./dwarfxml.o > toto.xml; xsltproc -o toto.html ../dwarf_basic.xslt  toto.xml 
 -->

<xsl:key name="dies" match="/dwarf/debug_info//*" use="@id"/>
<xsl:key name="files" match="/dwarf/debug_line//sl_file" use="@id"/>
<xsl:key name="rg_dies" match="/dwarf/debug_info//*[at_ranges]" use="concat('rg:', at_ranges)"/>

<xsl:template match="/">
  <html lang="en">
    <head>
     <title>DWARF debug information in <xsl:value-of select="/dwarf/@file" /></title>
     <meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1" />
     <meta name="description" content="An HTML view of DWARF debug information in ELF file {/dwarf/@file}" />
     <meta name="generator" content="{/dwarf/@producer} - {system-property('xsl:vendor')} - dwarf_basic.xslt" />
     <meta http-equiv="Content-Style-Type" content="text/css" />
    <link rel="stylesheet" type="text/css"  href="dwarf.css" />
<xsl:text>
</xsl:text><xsl:comment> postprod:header </xsl:comment><xsl:text>
</xsl:text>
    </head>
  <body>
<xsl:text>
</xsl:text><xsl:comment> postprod:top </xsl:comment><xsl:text>
</xsl:text>
    <h1>Dwarf debug informations content of <br /><xsl:value-of select="/dwarf/@file"/></h1>
    <h2><a name="toc" /> Dwarf sections</h2>
    <blockquote id="toc" >
      <big><big><big><xsl:apply-templates select="/dwarf/*" mode="toc"/></big></big></big>
    </blockquote>
    <xsl:apply-templates/><xsl:text>
</xsl:text><xsl:comment> postprod:signature </xsl:comment><xsl:text>
</xsl:text><xsl:comment> postprod:bottom </xsl:comment><xsl:text>
</xsl:text>
  <br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br />
  <br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br /><br />
  </body>
  </html>
</xsl:template>
<xsl:template match="/dwarf/*" mode="toc"><xsl:text>
  </xsl:text>
  <a href="#h2_{local-name()}"><xsl:value-of select="local-name()"/></a><xsl:text> </xsl:text>
</xsl:template>

<!-- Aranges -->
<!-- <arange id='ar:1' low_pc='0x08049614' length='2409' ref='i:685' /> -->
<xsl:template match="/dwarf/debug_aranges">
  <h2><a name ="h2_{local-name()}" />.debug_aranges - Lookup by adress</h2>
  <table border="1">
    <tr >
      <th>low pc</th>
      <th>length</th>
      <th>die</th>
      <th><i>name</i></th>
    </tr>
  <xsl:for-each select="arange">
    <tr>
      <td><xsl:value-of select="@low_pc"/></td>
      <td><xsl:value-of select="@length"/></td>
      <!-- <td><xsl:value-of select="substring-after(@ref,':')"/></td> -->
      <td><a href="#{@ref}"><xsl:value-of select="@ref"/></a></td>
      <td><i><a href="#{@ref}"><xsl:value-of select="key('dies',@ref)/at_name"/></a></i></td>
    </tr>
    </xsl:for-each>
  </table>
</xsl:template>


<!-- Info -->
<xsl:template match="/dwarf/debug_info">
  <h2><a name ="h2_{local-name()}" />.debug_info - Program scope entries</h2>
  <ul>
  <xsl:apply-templates/>
  </ul>
</xsl:template>
<xsl:template match="/dwarf/debug_info//*[@id and at_name]" priority = "-0.1" >
    <li><a name="{@id}" />off <xsl:value-of select="substring-after(@id,':')" /> - <b><a href="#{@ab}"><xsl:apply-templates select="." mode="print_tag_name" /></a> : <xsl:value-of select="at_name" /></b></li>
    <ul><xsl:apply-templates select="." mode="cu_version" /><xsl:apply-templates /></ul>
</xsl:template>
<xsl:template match="/dwarf/debug_info//*[@id and not(at_name)]" priority = "-0.1" >
    <li><a name="{@id}" />off <xsl:value-of select="substring-after(@id,':')" /> - <b><a href="#{@ab}"><xsl:apply-templates select="." mode="print_tag_name" /></a></b> : <i>#anonymous</i></li>
    <ul><xsl:apply-templates select="." mode="cu_version" /><xsl:apply-templates /></ul>
</xsl:template>
<xsl:template match="/dwarf/debug_info//at_name[../@id]" priority = "-0.1" ></xsl:template>
<xsl:template match="/dwarf/debug_info//*[@name]" mode="print_tag_name" ><xsl:value-of select="local-name()" />_<xsl:value-of select="@name" /></xsl:template>
<xsl:template match="/dwarf/debug_info//*[not(@name)]" mode="print_tag_name" ><xsl:value-of select="local-name()" /></xsl:template>

<xsl:template match="/dwarf/debug_info/tag_compile_unit[@version]" mode="cu_version" >
  <li>dwarf version : <xsl:value-of select="@version" /></li></xsl:template>
<xsl:template match="/dwarf/debug_info//*" mode="cu_version" priority = "-0.1" ></xsl:template>

<xsl:template match="/dwarf/debug_info//*[not(@id)]" priority = "-0.1" >
  <li><xsl:value-of select="local-name()" /> : <xsl:apply-templates/></li></xsl:template>

<xsl:template match="/dwarf/debug_info//at_macro_info" >
  <li><xsl:value-of select="local-name()" /> : <a href="#mac:{.}" >mac ref <xsl:value-of select="." /></a></li>
</xsl:template>
<xsl:template match="/dwarf/debug_info/tag_compile_unit/at_stmt_list" >
  <li><xsl:value-of select="local-name()" /> : <a href="#{../@sl}f">file table</a>, <a href="#{concat('sl:', .)}l" >lines table <xsl:value-of select="." /></a></li></xsl:template>

<xsl:template match="/dwarf/debug_info//at_type|at_sibling|at_import|at_abstract_origin|at_specification|at_namelist_item|at_friend|at_object_pointer|at_containing_type" >
  <li><xsl:value-of select="local-name()" /> : <a href="#{concat('i:',.)}">off <xsl:value-of select="." /></a>
  <xsl:text> </xsl:text>
  <i><a href="#{concat('i:',.)}"><xsl:value-of select="key('dies', concat('i:',.))/at_name" /></a></i></li></xsl:template>
<xsl:template match="/dwarf/debug_info//at_ranges" >
  <li><xsl:value-of select="local-name()" /> : <a href="#{concat('rg:',.)}">ranges <xsl:value-of select="." /></a></li></xsl:template>

<!-- <loc_ref ref='loc:0' /> -->
<xsl:template match="/dwarf/debug_info//loc_ref" >
   <a href="#{@ref}">loc list ref<xsl:text> </xsl:text><xsl:value-of select="substring-after(@ref,':')" /></a>
</xsl:template>
<xsl:template match="/dwarf/debug_info//loc_const" ><xsl:value-of select="." /></xsl:template>

<xsl:template match="/dwarf/debug_info//at_user|at_unk|at_out_range" >
   <li><xsl:value-of select="local-name()" /> "<xsl:value-of select="@name" />"  : <xsl:apply-templates/></li>
</xsl:template>

<xsl:template match="/dwarf/debug_info//at_decl_file[../at_decl_line and not(../at_decl_column)]" >
   <xsl:variable name="cu_sl" select="ancestor::tag_compile_unit/@sl" />
   <li>decl : <a href="#{concat($cu_sl,'f',.)}"><i><xsl:value-of select="key('files',concat($cu_sl,'f',.))" /></i></a>
    (<xsl:value-of select="."/>) at line <xsl:value-of select="../at_decl_line" /></li>
</xsl:template>
<xsl:template match="/dwarf/debug_info//at_decl_file[../at_decl_line and ../at_decl_column]" >
   <xsl:variable name="cu_sl" select="ancestor::tag_compile_unit/@sl" />
   <li>decl : <a href="#{concat($cu_sl,'f',.)}"><i><xsl:value-of select="key('files',concat($cu_sl,'f',.))" /></i></a>
    (<xsl:value-of select="."/>) at line <xsl:value-of select="../at_decl_line" />, col <xsl:value-of select="../at_decl_column" /></li>
</xsl:template>
<xsl:template match="/dwarf/debug_info//at_decl_line[../at_decl_file]" />
<xsl:template match="/dwarf/debug_info//at_decl_column[../at_decl_file and ../at_decl_line]" />

<xsl:template match="/dwarf/debug_info//loc_expr"><xsl:apply-templates /></xsl:template>

<xsl:template match="loc_expr/loc_op[@p2]">
   <xsl:text>[</xsl:text><xsl:value-of select="@pos"/><xsl:text>] </xsl:text>
   <xsl:value-of select="@op"/>(<xsl:value-of select="@p1"/>, <xsl:value-of select="@p2"/>)<xsl:text> </xsl:text>
</xsl:template>
<xsl:template match="loc_expr/loc_op[@p1 and not(@p2)]">
   <xsl:text>[</xsl:text><xsl:value-of select="@pos"/><xsl:text>] </xsl:text>
   <xsl:value-of select="@op"/>(<xsl:value-of select="@p1"/>)<xsl:text> </xsl:text>
</xsl:template>
<xsl:template match="loc_expr/loc_op[not(@p1)]">
   <xsl:text>[</xsl:text><xsl:value-of select="@pos"/><xsl:text>] </xsl:text>
   <xsl:value-of select="@op"/><xsl:text> </xsl:text>
</xsl:template>


<!-- debug_abbrev -->
<!--
     <ab_table id='ab:0'>
      <ab_tag name='tag_compile_unit' id='ab:0c1' children='true' >
        <ab_attr name='producer' form='strp' />
 -->
<xsl:template match="/dwarf/debug_abbrev">
   <h2><a name ="h2_{local-name()}" />.debug_abbrev - Format of debugging information</h2>
   <xsl:apply-templates select="./ab_table" /> 
</xsl:template>
<xsl:template match="/dwarf/debug_abbrev/ab_table">
<!--   <xsl:variable name="cu" select="@index +1"/>
  <h3>Compilation unit <xsl:value-of select="@index"/> 
      <xsl:value-of select="$cu"/>
      <xsl:value-of select="/dwarf/debug_info[compile_unit]/compile_unit[position()=$cu]/name" />
  </h3> -->
  <h3>Abbrev table at off <xsl:value-of select="substring-after(@id,':') "/> 
  for <a href='#{@ref}'>CU off <xsl:value-of select="substring-after(@ref,':') "/></a></h3>
  <table border="1">
    <tr >
      <th>tag name</th>
      <th>code</th>
      <th>child</th>
      <th>attributes</th>
    </tr>
  <xsl:for-each select="ab_tag">
  <tr>
   <td><a name="{@id}" /><xsl:value-of select="@name"/></td>
   <td><xsl:value-of select="substring-after(@id,'c')"/></td>
   <td><xsl:value-of select="@children"/></td>
   <td><xsl:apply-templates select="ab_attr"/></td>
  </tr>
  </xsl:for-each>
  </table> 
</xsl:template>
<xsl:template match="/dwarf/debug_abbrev/ab_table/ab_tag/ab_attr">
   <xsl:value-of select="@name"/> (<xsl:value-of select="@form"/>), 
</xsl:template>

<!-- 
  <debug_line>
    <sl_compile_unit id='sl:0' ref='i:11'>
      <sl_files id='sl:0f'>
        <sl_file id='sl:0f1'>/home/manu/projets/decompile/dwarf2xml/src/test_dwarf_type.c</sl_file>
      </sl_files>
      <sl_lines id='sl:0l'>
        <sl_line id='sl:0l1' ref='i:11'>
          <sl_low_pc>0x080484a4</sl_low_pc>
          <sl_decl file='1' line='32'/>
          <sl_properties stm_start='true' block_start='false' seq_end='false' />
-->
<xsl:template match="/dwarf/debug_line">
   <h2><a name ="h2_{local-name()}" />.debug_line - Line number information</h2>
   <xsl:apply-templates select="./sl_compile_unit" />   
</xsl:template>
<xsl:template match="/dwarf/debug_line/sl_compile_unit">
 <h3><a name="{@id}" />Source info at <xsl:value-of select="substring-after(@id,':')"/> 
     for CU <a href="#{@ref}">off <xsl:value-of select="substring-after(@ref,':')"/></a></h3>
 <h4><a name="{sl_files/@id}" />file table index</h4>
  <table border="1">
    <tr >
      <th>index</th>
      <th>file</th>
    </tr>
  <xsl:for-each select="sl_files/sl_file">
  <tr>
   <td><a name="{@id}" /><xsl:value-of select="substring-after(@id,'f')"/></td>
   <td><xsl:value-of select="."/></td>
  </tr>
  </xsl:for-each>
  </table> 

 <h4><a name="{sl_lines/@id}" />Source line information</h4>
  <table border="1">
    <tr >
      <th>PC</th>
      <th>file</th>
      <th>line</th>
      <th>col</th>
      <th>stm_start</th>
      <th>seq_end</th>
      <th>block_start</th>
      <th><i>die</i></th>
    </tr>
  <xsl:for-each select="sl_lines/sl_line">
  <tr>
   <td><xsl:value-of select="sl_low_pc" /></td>
   <td><a href="#{sl_decl/@file}"><xsl:value-of select="key('files',sl_decl/@file)" /></a></td> 
   <td><xsl:value-of select="sl_decl/@line" /></td>
   <td><xsl:value-of select="sl_decl/@col" /></td>
   <td><xsl:value-of select="sl_properties/@stm_start" /></td>
   <td><xsl:value-of select="sl_properties/@seq_end" /></td>
   <td><xsl:value-of select="sl_properties/@block_start" /></td>
   <td><xsl:apply-templates select="." /></td>
  </tr>
  </xsl:for-each>
  </table> 
</xsl:template>
<xsl:template match="/dwarf/debug_line//sl_line[@ref]">
   <i><a href="#{@ref}"><xsl:value-of select="key('dies',@ref)/at_name" /> off <xsl:value-of select="substring-after(@ref,':')" /></a></i>
</xsl:template>
<xsl:template match="/dwarf/debug_line//sl_line[not(@ref)]">
    <br />
</xsl:template>

<!--  
  <debug_frame>
    <fr_cie id='fr:c0' version='1' augmenter='' >
      <fr_alignment_factor code='1' data='-4' />
      <fr_return_address register_rule='8' />
      <fr_initial_instructions>
        <fr_instr pc_off='+0' op='def_cfa' p1='4' p2='4' />
      </fr_initial_instructions>
      <fr_fde id='fr:c0f20' ref='i:11' low_pc='0x080484a4' func_length='25' >
        <fr_instr pc_off='+0' op='advance_loc' p1='0' p2='4' />
 -->  
<xsl:template match="/dwarf/debug_frame">
  <h2><a name ="h2_{local-name()}" />.debug_frame - Call frame information</h2>
  <xsl:apply-templates select="./fr_cie" />
</xsl:template>
<xsl:template match="/dwarf/eh_frame">
  <h2><a name ="h2_{local-name()}" />.eh_frame - GNU exception frame information</h2>
  <xsl:apply-templates select="./fr_cie" />
</xsl:template>

<xsl:template match="/dwarf//fr_cie">
  <h3><a name="{@id}" />CIE #<xsl:value-of select="substring-after(@id,'c')"/> - Common information entry</h3>
  <table border="1">
    <tr >
      <th>version</th>
      <th>augmenter</th>
      <th>code_alignment_factor</th>
      <th>data_alignment_factor</th>
      <th>return_address_register_rule</th>
    </tr>
  <tr>
   <td><xsl:value-of select="@version"/></td>
   <td><xsl:value-of select="@augmenter"/></td>
   <td><xsl:value-of select="fr_alignment_factor/@code"/></td>
   <td><xsl:value-of select="fr_alignment_factor/@data"/></td>
   <td><xsl:value-of select="fr_return_address/@register_rule"/></td>
  </tr>
  </table> 
  <ul>
  <xsl:apply-templates select="./fr_initial_instructions" />
  </ul>
  <xsl:apply-templates select="./fr_fde" />
</xsl:template>
<xsl:template match="/dwarf//fr_cie/fr_fde">
  <h4><a name="{@id}" />FDE #<xsl:value-of select="substring-after(substring-after(@id,':'),'f')"/> - Frame description entry</h4>
  <table border="1">
    <tr >
      <th>Cie</th>
      <th>low_pc</th>
      <th>func_length</th>
      <th><i>function</i></th>
    </tr>
  <tr><xsl:variable name="f_addr" select="low_pc"/>
   <td><a href="#{../@id}"><xsl:value-of select="substring-after(../@id,'c')"/></a></td>
   <td><xsl:value-of select="@low_pc"/></td>
   <td><xsl:value-of select="@func_length"/></td>
   <td><i><a href="#{@ref}"><xsl:value-of select="key('dies',@ref)/at_name"/> off <xsl:value-of select="substring-after(key('dies',@ref)/@id,':')"/></a></i></td>
  </tr>
  </table>
  <ul>
  <xsl:apply-templates select="./fr_instr" />
  </ul>
</xsl:template>
<xsl:template match="/dwarf//fr_instr">
   <li><xsl:value-of select="@pc_off"/><xsl:text> : </xsl:text><xsl:value-of select="@op"/>(<xsl:value-of select="@p1"/>, <xsl:value-of select="@p2"/>)</li>
</xsl:template>

<!-- Location list
  <debug_loc>
    <loc_list>
      <loc_expr id='loc:0' low_pc='0x00000000' high_pc='0x00000004' >
        <loc_op pos='0' op='breg4' p1='0x4'/>
-->
<xsl:template match="/dwarf/debug_loc">
  <h2><a name ="h2_{local-name()}" />.debug_loc - Location lists</h2>
  <xsl:for-each select="loc_list">
  <ul>
  <xsl:for-each select="loc_expr">
   <li>
     <a name="{@id}" /><xsl:text>PC range </xsl:text>
     <xsl:value-of select="@low_pc"/> .. <xsl:value-of select="@high_pc"/> :
     <xsl:apply-templates select="loc_op" />
   </li>
  </xsl:for-each>
  </ul>
  </xsl:for-each>
</xsl:template>

<xsl:template match="/dwarf/debug_loc//loc_expr">
   PC <xsl:value-of select="low_pc"/> .. <xsl:value-of select="high_pc"/>: <xsl:apply-templates select="loc_op" />
</xsl:template>
<!-- 
  <debug_ranges>
    <rg_range id='rg:0' >
      <rg_entry low_pc='0x00000040' high_pc='0x00000046' />
      <rg_entry low_pc='0x00000056' high_pc='0x0000006a' />
      <rg_entry />
    </rg_range>
    <rg_range id='rg:24' >
-->
<xsl:template match="/dwarf/debug_ranges">
  <h2><a name ="h2_{local-name()}" />.debug_ranges - Non-Contiguous Address Ranges</h2>
  <ul>
  <xsl:for-each select="rg_range">
  <xsl:variable name="ref" select="key('rg_dies',@id)/@id"/>
  <li><a name ="{@id}" />Ranges at off <xsl:value-of select="substring-after(@id,':')"/> 
  for die <a href="#{$ref}"><xsl:value-of select="$ref/at_name"/> <xsl:value-of select="substring-after($ref,':')"/></a></li>
  <ul>
  <xsl:apply-templates  select="rg_entry"/>
  </ul>
  </xsl:for-each>
  </ul>
</xsl:template>
<xsl:template match="rg_entry[@high_pc]">
   <li><xsl:text>rel PC in </xsl:text><xsl:value-of select="@low_pc"/> .. <xsl:value-of select="@high_pc"/></li>
</xsl:template>
<xsl:template match="rg_entry[@lo_pc and not(@high_pc)]">
   <li><xsl:text>rel PC at </xsl:text><xsl:value-of select="@low_pc"/></li>
</xsl:template>
<xsl:template match="rg_entry[not(@lo_pc) and not(@high_pc)]" />

<!-- debug_macinfo 
  <debug_macinfo>
    <mac_compile_unit id='mac:0' ref='i:11'>
      <macro id='mac:0m1' file='1' line='0' >#start_file</macro>
      <macro id='mac:0m2' file='1' line='1' >#define __STDC__ 1</macro>
-->
<xsl:template match="/dwarf/debug_macinfo">
  <h2><a name ="h2_{local-name()}" />.debug_macinfo - Macro information</h2>
   <xsl:apply-templates select="./mac_compile_unit" />   
</xsl:template>
<xsl:template match="/dwarf/debug_macinfo/mac_compile_unit">
  <h3><a name="{@id}" />Macros at <xsl:value-of select="substring-after(@id,':')"/> 
       for CU <a href="#{@ref}" >off <xsl:value-of select="substring-after(@ref,':')"/></a></h3>
  <table border="1">
    <tr >
      <th>fileno</th>
      <th>lineno</th>
      <th>type</th>
      <th>value</th>
    </tr>
  <xsl:for-each select="macro">
  <tr><a name="{id}" />
   <td><a href="#{concat(key('dies', ../@ref)/@sl,'f', @file)}"><xsl:value-of select="@file"/></a></td>
   <td><xsl:value-of select="@line"/></td>
   <td><xsl:value-of select="@type"/></td>
   <td><xsl:value-of select="."/></td>
  </tr>
  </xsl:for-each>
  </table> 

</xsl:template>

<!-- debug_pubnames
    <pubname id='pub:2' cu='i:11' ref='i:215'>var_named_enum</pubname>
-->
<xsl:template match="/dwarf/debug_pubnames">
  <h2><a name ="h2_{local-name()}" />.debug_pubnames - Public names</h2>
  <ul>
  <xsl:for-each select="pubname">
  <li><a name="{@id}" /> 
   cu <a href="#{@cu}"><xsl:value-of select="@cu"/></a>, 
   die <a href="#{@ref}"><xsl:value-of select="@ref"/></a> 
   <xsl:text> : </xsl:text><xsl:value-of select="."/>
  </li>
  </xsl:for-each>
  </ul> 
</xsl:template>

<!-- <debug_pubtypes>
       <pubtype id="pt:1" cu="i:11" ref="i:138">Dwarf_Unsigned</pubtype>
-->
<xsl:template match="/dwarf/debug_pubtypes">
  <h2><a name ="h2_{local-name()}" />.debug_pubtypes - Public types</h2>
  <ul>
  <xsl:for-each select="pubtype">
  <li><a name="{@id}" /> 
   cu <a href="#{@cu}"><xsl:value-of select="@cu"/></a>, 
   die <a href="#{@ref}"><xsl:value-of select="@ref"/></a> 
   <xsl:text> : </xsl:text><xsl:value-of select="."/>
  </li>
  </xsl:for-each>
  </ul> 
</xsl:template>

<!-- <debug_weaknames>
       <weakname id="wn:1" cu="i:219094" ref="i:223846">elf_strptr_xtnd</weakname>
-->
<xsl:template match="/dwarf/debug_weaknames">
  <h2><a name ="h2_{local-name()}" />.debug_weaknames - Weak names</h2>
  <ul>
  <xsl:for-each select="weakname">
  <li><a name="{@id}" /> 
   cu <a href="#{@cu}"><xsl:value-of select="@cu"/></a>, 
   die <a href="#{@ref}"><xsl:value-of select="@ref"/></a> 
   <xsl:text> : </xsl:text><xsl:value-of select="."/>
  </li>
  </xsl:for-each>
  </ul> 
</xsl:template>

<!-- <debug_varnames>
       <varname id="vn:1" cu="i:414" ref="i:1254">program_name</varname>
-->
<xsl:template match="/dwarf/debug_varnames">
  <h2><a name ="h2_{local-name()}" />.debug_varnames - Variable names</h2>
  <ul>
  <xsl:for-each select="varname">
  <li><a name="{@id}" /> 
   cu <a href="#{@cu}"><xsl:value-of select="@cu"/></a>, 
   die <a href="#{@ref}"><xsl:value-of select="@ref"/></a> 
   <xsl:text> : </xsl:text><xsl:value-of select="."/>
  </li>
  </xsl:for-each>
  </ul> 
</xsl:template>

<!-- <debug_funcnames>
       <funcname id="fn:1" cu="i:414" ref="i:4540">open_a_file</funcname>
-->
<xsl:template match="/dwarf/debug_funcnames">
  <h2><a name ="h2_{local-name()}" />.debug_funcnames - Function names</h2>
  <ul>
  <xsl:for-each select="funcname">
  <li><a name="{@id}" /> 
   cu <a href="#{@cu}"><xsl:value-of select="@cu"/></a>, 
   die <a href="#{@ref}"><xsl:value-of select="@ref"/></a> 
   <xsl:text> : </xsl:text><xsl:value-of select="."/>
  </li>
  </xsl:for-each>
  </ul> 
</xsl:template>

<!-- <debug_typenames>
       <typename id="tn:1" cu="i:414" ref="i:890">__file_s</typename>
-->
<xsl:template match="/dwarf/debug_typenames">
  <h2><a name ="h2_{local-name()}" />.debug_typenames - Type names</h2>
  <ul>
  <xsl:for-each select="typename">
  <li><a name="{@id}" /> 
   cu <a href="#{@cu}"><xsl:value-of select="@cu"/></a>, 
   die <a href="#{@ref}"><xsl:value-of select="@ref"/></a> 
   <xsl:text> : </xsl:text><xsl:value-of select="."/>
  </li>
  </xsl:for-each>
  </ul> 
</xsl:template>
</xsl:stylesheet>

