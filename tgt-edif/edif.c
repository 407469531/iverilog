/*
 * Copyright (c) 2005 Stephen Williams
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifdef HAVE_CVS_IDENT
#ident "$Id: edif.c,v 1.1.2.3 2006/11/11 21:21:21 steve Exp $"
#endif

# include  "edif.h"
# include  <stdlib.h>
# include  <string.h>
#ifdef HAVE_MALLOC_H
# include  <malloc.h>
#endif
# include  <assert.h>

typedef enum property_e {
      PRP_NONE = 0,
      PRP_STRING,
      PRP_INTEGER
} property_t;

struct cellref_property_ {
      const char*name;
      property_t ptype;
      union {
	    const char*str;
	    long num;
      } value_;
      struct cellref_property_*next;
};

/*
 * This is the root of an EDIF design. From here we can get at the
 * root ports of the design (which are typically the pins of a chip or
 * FPGA design) a list of external cell libraries, cells properties
 * and nexa (joints).
 */
struct edif_s {
      const char*name;
	/* List the ports of the design. */
      unsigned nports;
      struct __cell_port*ports;
	/* All the external libraries attached to me. */
      edif_xlibrary_t xlibs;
	/* list the cellref instances. */
      edif_cellref_t celref;
	/* The root instance has cellref properties as well. */
      struct cellref_property_*property;
	/* Keep a list of all the nexa */
      struct edif_joint_s*nexa;
};

struct edif_xlibrary_s {
	/* Name of this library. */
      const char*name;
	/* The cells that are contained in this library. */
      struct edif_cell_s*cells;
	/* point to the optional celltable. */
      const struct edif_xlib_celltable*celltable;
	/* used to list libraries in an edif_t. */
      struct edif_xlibrary_s*next;
};


struct __cell_port {
      const char*name;
      const char*ename;
      struct cellref_property_*property;
      ivl_signal_port_t dir;
};

struct edif_cell_s {
      const char*name;
      edif_xlibrary_t xlib;

      unsigned nports;
      struct __cell_port*ports;

      struct cellref_property_*property;
      struct edif_cell_s*next;
};

struct edif_cellref_s {
      struct edif_cell_s* cell;
      unsigned u;
      struct cellref_property_*property;
      struct edif_cellref_s* next;
};

struct joint_cell_ {
      struct edif_cellref_s*cell;
      unsigned port;
      struct joint_cell_*next;
};

/*
 * Joints link cell ports together. The edif_t object contains a list
 * of these joints, and each joint lists the ports that are connected
 * together. The port is represented by a joint_cell_.
 *
 * The links list a singly linked list of joint_cell_ objects. To add
 * a new link, simply push a new joint_cell_ into the links list. See
 * the edif_add_to_joint and edif_port_to_joint functions.
 *
 * The joints themselves are kept in a singly linked list in the
 * edif_t object. To create a new joint, simply push an initialized
 * edif_joint_s onto the nexa member of the edif_t object. See the
 * edif_joint_create function.
 */
struct edif_joint_s {
      const char*name;
      struct joint_cell_*links;
      struct edif_joint_s*next;
};


static int is_edif_name(const char*text)
{
      static const char*edif_name_chars = "abcdefghijklmnopqrstuvwxyz"
                                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                          "0123456789";
      return (strspn(text, edif_name_chars) == strlen(text));
}

edif_t edif_create(const char*design_name, unsigned nports)
{
      edif_t edf = malloc(sizeof(struct edif_s));

      edf->name  = design_name;
      edf->nports= nports;
      edf->ports = nports? calloc(nports, sizeof(struct __cell_port)) : 0;
      edf->celref= 0;
      edf->xlibs = 0;
      edf->property = 0;
      edf->nexa  = 0;

      return edf;
}

void edif_portconfig(edif_t edf, unsigned idx,
		     const char*name, ivl_signal_port_t dir)
{
      assert(idx < edf->nports);

      edf->ports[idx].name = name;
      if (is_edif_name(name)) {
	    edf->ports[idx].ename = 0;

      } else {
	    char buf[16];
	    sprintf(buf, "PORT%u", idx);
	    edf->ports[idx].ename = strdup(buf);
      }

      edf->ports[idx].dir  = dir;
}

void edif_port_to_joint(edif_joint_t jnt, edif_t edf, unsigned port)
{
      struct joint_cell_* jc = malloc(sizeof(struct joint_cell_));

      jc->cell = 0;
      jc->port = port;
      jc->next = jnt->links;
      jnt->links = jc;
}

void edif_pstring(edif_t edf, const char*name, const char*value)
{
      struct cellref_property_*prp = malloc(sizeof(struct cellref_property_));
      prp->name = name;
      prp->ptype = PRP_STRING;
      prp->value_.str = value;
      prp->next = edf->property;
      edf->property = prp;
}

edif_xlibrary_t edif_xlibrary_create(edif_t edf, const char*name)
{
      edif_xlibrary_t xlib = malloc(sizeof(struct edif_xlibrary_s));

      xlib->name  = name;
      xlib->cells = 0;
      xlib->celltable = 0;
      xlib->next = edf->xlibs;
      edf->xlibs = xlib;

      return xlib;
}

void edif_xlibrary_set_celltable(edif_xlibrary_t xlib,
				 const struct edif_xlib_celltable*tab)
{
      assert(xlib->celltable == 0);
      xlib->celltable = tab;
}

edif_cell_t edif_xlibrary_findcell(edif_xlibrary_t xlib,
				   const char*cell_name)
{
      const struct edif_xlib_celltable*tcur;
      edif_cell_t cur;

      for (cur = xlib->cells ;  cur ;  cur = cur->next) {
	    if (strcmp(cell_name, cur->name) == 0)
		  return cur;
      }

      if (xlib->celltable == 0)
	    return 0;

      for (tcur = xlib->celltable ;  tcur->cell_name ;  tcur += 1)
	    if (strcmp(cell_name, tcur->cell_name) == 0) {
		  return (tcur->cell_func)(xlib);
	    }

      return 0;
}

edif_cell_t edif_xlibrary_scope_cell(edif_xlibrary_t xlib,
				     ivl_scope_t scope)
{
      unsigned port_count, idx;
      edif_cell_t cur;

	/* Check to see if the cell is already somehow defined. */
      cur = edif_xlibrary_findcell(xlib, ivl_scope_tname(scope));
      if (cur) return cur;

	/* Count the ports of the scope. */
      port_count = 0;
      for (idx = 0 ;  idx < ivl_scope_sigs(scope) ;  idx += 1) {
	    ivl_signal_t sig = ivl_scope_sig(scope, idx);

	    if (ivl_signal_port(sig) == IVL_SIP_NONE)
		  continue;

	    port_count += 1;
      }

      cur = edif_xcell_create(xlib, ivl_scope_tname(scope), port_count);

      port_count = 0;
      for (idx = 0 ;  idx < ivl_scope_sigs(scope) ;  idx += 1) {
	    ivl_signal_t sig = ivl_scope_sig(scope, idx);

	    if (ivl_signal_port(sig) == IVL_SIP_NONE)
		  continue;

	    edif_cell_portconfig(cur, port_count,
				 ivl_signal_basename(sig),
				 ivl_signal_port(sig));
	    port_count += 1;
      }

      return cur;
}

edif_cell_t edif_xcell_create(edif_xlibrary_t xlib, const char*name,
			      unsigned nports)
{
      unsigned idx;
      edif_cell_t cell = malloc(sizeof(struct edif_cell_s));

      cell->name = name;
      cell->xlib = xlib;
      cell->nports = nports;
      cell->ports  = calloc(nports, sizeof(struct __cell_port));
      cell->property = 0;

      for (idx = 0 ;  idx < nports ;  idx += 1) {
	    cell->ports[idx].name = "?";
	    cell->ports[idx].dir = IVL_SIP_NONE;
	    cell->ports[idx].property = 0;
      }

      cell->next = xlib->cells;
      xlib->cells = cell;

      return cell;
}

void edif_cell_portconfig(edif_cell_t cell, unsigned idx,
			  const char*name, ivl_signal_port_t dir)
{
      assert(idx < cell->nports);

      cell->ports[idx].name = name;
      cell->ports[idx].dir  = dir;
}

void edif_cell_port_pstring(edif_cell_t cell, unsigned idx,
			    const char*name, const char*value)
{
      struct cellref_property_*prp = malloc(sizeof(struct cellref_property_));
      prp->name  = name;
      prp->ptype = PRP_STRING;
      prp->value_.str = value;
      prp->next  = cell->ports[idx].property;
      cell->ports[idx].property = prp;
}

unsigned edif_cell_port_byname(edif_cell_t cell, const char*name)
{
      unsigned idx = 0;
      for (idx = 0 ;  idx < cell->nports ;  idx += 1) {
	    if (cell->ports[idx].ename) {
		  if (strcmp(name, cell->ports[idx].ename) == 0)
			break;
	    } else {
		  if (strcmp(name, cell->ports[idx].name) == 0)
			break;
	    }
      }

      return idx;
}

void edif_cell_pstring(edif_cell_t cell, const char*name,
		       const char*value)
{
      struct cellref_property_*prp = malloc(sizeof(struct cellref_property_));
      prp->name  = name;
      prp->ptype = PRP_STRING;
      prp->value_.str = value;
      prp->next  = cell->property;
      cell->property = prp;
}

void edif_cell_pinteger(edif_cell_t cell, const char*name,
			int value)
{
      struct cellref_property_*prp = malloc(sizeof(struct cellref_property_));
      prp->name  = name;
      prp->ptype = PRP_INTEGER;
      prp->value_.num = value;
      prp->next  = cell->property;
      cell->property = prp;
}

edif_cellref_t edif_cellref_create(edif_t edf, edif_cell_t cell)
{
      static unsigned u_number = 0;
      edif_cellref_t ref = malloc(sizeof(struct edif_cellref_s));

      u_number += 1;

      assert(cell);
      assert(edf);

      ref->u = u_number;
      ref->cell = cell;
      ref->property = 0;
      ref->next = edf->celref;
      edf->celref = ref;

      return ref;
}

void edif_cellref_pstring(edif_cellref_t ref, const char*name,
			   const char*value)
{
      struct cellref_property_*prp = malloc(sizeof(struct cellref_property_));
      prp->name = name;
      prp->ptype = PRP_STRING;
      prp->value_.str = value;
      prp->next = ref->property;
      ref->property = prp;
}

void edif_cellref_pinteger(edif_cellref_t ref, const char*name, int value)
{
      struct cellref_property_*prp = malloc(sizeof(struct cellref_property_));
      prp->name = name;
      prp->ptype = PRP_INTEGER;
      prp->value_.num = value;
      prp->next = ref->property;
      ref->property = prp;
}

edif_joint_t edif_joint_create(edif_t edf)
{
      edif_joint_t jnt = malloc(sizeof(struct edif_joint_s));

      jnt->name = 0;
      jnt->links = 0;
      jnt->next  = edf->nexa;
      edf->nexa  = jnt;
      return jnt;
}

edif_joint_t edif_joint_of_nexus(edif_t edf, ivl_nexus_t nex)
{
      void*tmp = ivl_nexus_get_private(nex);
      edif_joint_t jnt;

      if (tmp == 0) {
	    jnt = edif_joint_create(edf);
	    ivl_nexus_set_private(nex, jnt);
	    return jnt;
      }

      jnt = (edif_joint_t) tmp;
      return jnt;
}

void edif_nexus_to_joint(edif_t edf, edif_joint_t jnt, ivl_nexus_t nex)
{
      void*tmp = ivl_nexus_get_private(nex);

      if (tmp != 0) {
	      /* There is a joint already on the nexus. Move all the
		 joint cells to the joint I'm joining to. */
	    edif_joint_t njnt = (edif_joint_t)tmp;
	    while (njnt->links) {
		  struct joint_cell_*cell = njnt->links;
		  njnt->links = cell->next;
		  cell->next = jnt->links;
		  jnt->links = cell;
	    }

	      /* Now njnt is dead, and should be removed from edif. */
	      /* Or we can ignore it as harmless. */
      }

      ivl_nexus_set_private(nex, jnt);
}

void edif_joint_rename(edif_joint_t jnt, const char*name)
{
      assert(jnt->name == 0);
      jnt->name = name;
}

void edif_add_to_joint(edif_joint_t jnt, edif_cellref_t cell, unsigned port)
{
      struct joint_cell_* jc = malloc(sizeof(struct joint_cell_));

      jc->cell = cell;
      jc->port = port;
      jc->next = jnt->links;
      jnt->links = jc;
}

static void fprint_property(FILE*fd, const struct cellref_property_*prp)
{
      fprintf(fd, "(property %s ", prp->name);
      switch (prp->ptype) {
	  case PRP_NONE:
	    break;
	  case PRP_STRING:
	    fprintf(fd, "(string \"%s\")", prp->value_.str);
	    break;
	  case PRP_INTEGER:
	    fprintf(fd, "(integer %ld)", prp->value_.num);
	    break;
      }
      fprintf(fd, ")");
}

/*
 * This function takes all the data structures that have been
 * assembled by the code generator, and writes them into an EDIF
 * formatted file.
 */
void edif_print(FILE*fd, edif_t edf)
{
      edif_xlibrary_t xlib;
      edif_cell_t cell;
      edif_cellref_t ref;
      edif_joint_t jnt;
      struct cellref_property_*prp;
      unsigned idx;

      fprintf(fd, "(edif %s\n", edf->name);
      fprintf(fd, "    (edifVersion 2 0 0)\n");
      fprintf(fd, "    (edifLevel 0)\n");
      fprintf(fd, "    (keywordMap (keywordLevel 0))\n");
      fprintf(fd, "    (status\n");
      fprintf(fd, "     (written\n");
      fprintf(fd, "        (timeStamp 0 0 0 0 0 0)\n");
      fprintf(fd, "        (author \"unknown\")\n");
      fprintf(fd, "        (program \"Icarus Verilog/edif.tgt\")))\n");
      fflush(fd);

      for (xlib = edf->xlibs ;  xlib ;  xlib = xlib->next) {

	    fprintf(fd, "    (external %s "
		    "(edifLevel 0) "
		    "(technology (numberDefinition))\n",
		    xlib->name);

	    for (cell = xlib->cells ;  cell ;  cell = cell->next) {
		  fprintf(fd, "      (cell %s (cellType GENERIC)\n",
			  cell->name);
		  fprintf(fd, "            (view net\n"
			      "              (viewType NETLIST)\n"
			      "              (interface");

		  for (idx = 0 ;  idx < cell->nports ;  idx += 1) {
			struct __cell_port*pp = cell->ports + idx;
			fprintf(fd, "\n                (port %s", pp->name);
			switch (pp->dir) {
			    case IVL_SIP_INPUT:
			      fprintf(fd, " (direction INPUT)");
			      break;
			    case IVL_SIP_OUTPUT:
			      fprintf(fd, " (direction OUTPUT)");
			      break;
			    case IVL_SIP_INOUT:
			      fprintf(fd, " (direction INOUT)");
			      break;
			    default:
			      break;
			}

			for (prp = pp->property ;  prp ;  prp=prp->next) {
			      fprintf(fd, " ");
			      fprint_property(fd, prp);
			}

			fprintf(fd, ")");
		  }

		  for (prp = cell->property ;  prp ;  prp = prp->next) {
			fprintf(fd, "\n                ");
			fprint_property(fd, prp);
		  }
		  fprintf(fd, ")))\n");
	    }

	    fprintf(fd, "    )\n"); /* terminate (external ...) sexp */
      }
      fflush(fd);

	/* Write out the library header */
      fprintf(fd, "    (library DESIGN\n");
      fprintf(fd, "      (edifLevel 0)\n");
      fprintf(fd, "      (technology (numberDefinition))\n");

	/* The root module is a cell in the library. */
      fprintf(fd, "      (cell %s\n", edf->name);
      fprintf(fd, "        (cellType GENERIC)\n");
      fprintf(fd, "        (view net\n");
      fprintf(fd, "          (viewType NETLIST)\n");
      fprintf(fd, "          (interface\n");

      for (idx = 0 ;  idx < edf->nports ;  idx += 1) {
	    fprintf(fd, "            (port ");
	    if (edf->ports[idx].ename == 0)
		  fprintf(fd, "%s ", edf->ports[idx].name);
	    else
		  fprintf(fd, "(rename %s \"%s\") ",
			  edf->ports[idx].ename,
			  edf->ports[idx].name);

	    switch (edf->ports[idx].dir) {
		case IVL_SIP_INPUT:
		  fprintf(fd, "(direction INPUT)");
		  break;
		case IVL_SIP_OUTPUT:
		  fprintf(fd, "(direction OUTPUT)");
		  break;
		case IVL_SIP_INOUT:
		  fprintf(fd, "(direction INOUT)");
		  break;
		default:
		  break;
	    }
	    fprintf(fd, ")\n");
      }

      fprintf(fd, "          )\n"); /* end the (interface ) sexp */
      fflush(fd);

      fprintf(fd, "          (contents\n");

	/* Display all the instances. */
      for (ref = edf->celref ;  ref ;  ref = ref->next) {

	    assert(ref->cell);

	    fprintf(fd, "(instance U%u (viewRef net "
		    "(cellRef %s (libraryRef %s)))",
		    ref->u, ref->cell->name, ref->cell->xlib->name);

	    for (prp = ref->property ;  prp ;  prp = prp->next) {
		  fprintf(fd, " ");
		  fprint_property(fd, prp);
	    }

	    fprintf(fd, ")\n");
      }

      fflush(fd);

	/* Display all the joints. */
      idx = 0;
      for (jnt = edf->nexa ;  jnt ;  jnt = jnt->next, idx += 1) {
	    struct joint_cell_*jc;

	      /* Skip nil joints. */
	    if (jnt->links == 0)
		  continue;

	    fprintf(fd, "(net ");
	    if (jnt->name != 0)
		  fprintf(fd, "(rename N%u \"%s\")", idx, jnt->name);
	    else
		  fprintf(fd, "N%u", idx);
	    fprintf(fd, " (joined");

	    for (jc = jnt->links ;  jc ;  jc = jc->next) {
		  if (jc->cell) {
			fprintf(fd, " (portRef %s (instanceRef U%u))",
				jc->cell->cell->ports[jc->port].name,
				jc->cell->u);
		  } else {
			  /* Reference to a port of the main cell. */
			if (edf->ports[jc->port].ename)
			      fprintf(fd, " (portRef %s)",
				      edf->ports[jc->port].ename);
			else
			      fprintf(fd, " (portRef %s)",
				      edf->ports[jc->port].name);
		  }
	    }
	    fprintf(fd, "))\n");
      }

      fprintf(fd, "          )\n"); /* end the (contents...) sexp */

      fprintf(fd, "        )\n"); /* end the (view ) sexp */
      fprintf(fd, "      )\n"); /* end the (cell ) sexp */
      fprintf(fd, "    )\n"); /* end the (library DESIGN) sexp */

	/* Make an instance of the defined object */
      fprintf(fd, "    (design %s\n", edf->name);
      fprintf(fd, "      (cellRef %s (libraryRef DESIGN))\n", edf->name);

      for (prp = edf->property ;  prp ;  prp = prp->next) {
	    fprintf(fd, "       ");
	    fprint_property(fd, prp);
	    fprintf(fd, "\n");
      }

      fprintf(fd, "    )\n");



      fprintf(fd, ")\n");
      fflush(fd);
}

/*
 * $Log: edif.c,v $
 * Revision 1.1.2.3  2006/11/11 21:21:21  steve
 *  Make signal lookup use extended names.
 *
 * Revision 1.1.2.2  2005/08/21 22:25:51  steve
 *  Fix the comment in the EDIT header.
 *
 * Revision 1.1.2.1  2005/08/17 01:17:28  steve
 *  Add the tgt-edif target.
 *
 */

