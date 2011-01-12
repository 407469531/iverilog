/*
 * Copyright (c) 1998-2010 Stephen Williams <steve@icarus.com>
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

# include "config.h"

# include  <iostream>

# include  "target.h"
# include  <typeinfo>

target_t::~target_t()
{
}

void target_t::scope(const NetScope*)
{
}

void target_t::event(const NetEvent*ev)
{
      cerr << ev->get_line() << ": error: target (" << typeid(*this).name()
	   <<  "): Unhandled event <" << ev->full_name() << ">." << endl;
}

void target_t::memory(const NetMemory*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled memory." << endl;
}

void target_t::variable(const NetVariable*that)
{
      cerr << that->get_line() << ": error: target (" << typeid(*this).name()
	   <<  "): Unhandled variable <" << that->basename() << ">." << endl;
}

bool target_t::func_def(const NetScope*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled function definition." << endl;
      return false;
}

void target_t::task_def(const NetScope*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled task definition." << endl;
}

void target_t::logic(const NetLogic*)
{
}

bool target_t::bufz(const NetBUFZ*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled continuous assign (BUFZ)." << endl;
      return false;
}

void target_t::udp(const NetUDP*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled UDP." << endl;
}

void target_t::lpm_add_sub(const NetAddSub*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetAddSub." << endl;
}

void target_t::lpm_clshift(const NetCLShift*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetCLShift." << endl;
}

void target_t::lpm_compare(const NetCompare*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetCompare." << endl;
}

bool target_t::lpm_decode(const NetDecode*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetDecode." << endl;
      return false;
}

bool target_t::lpm_demux(const NetDemux*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetDemux." << endl;
      return false;
}

void target_t::lpm_divide(const NetDivide*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetDivide." << endl;
}

void target_t::lpm_modulo(const NetModulo*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetModulo." << endl;
}

void target_t::lpm_ff(const NetFF*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetFF." << endl;
}

void target_t::lpm_latch( const NetLatch * )
{
      cerr << "target (" << typeid( *this ).name() << "): Unhandled NetLatch." << endl;
}

void target_t::lpm_mult(const NetMult*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetMult." << endl;
}

void target_t::lpm_mux(const NetMux*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetMux." << endl;
}

void target_t::lpm_ram_dq(const NetRamDq*)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled NetRamDq." << endl;
}

void target_t::net_case_cmp(const NetCaseCmp*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled case compare node." << endl;
}

bool target_t::net_cassign(const NetCAssign*dev)
{
	cerr << "target (" << typeid(*this).name() <<  "): ";
	cerr << dev->get_line();
	cerr << ": Target does not support procedural continuous assignment." << endl;
      return false;
}

bool target_t::net_const(const NetConst*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled CONSTANT node." << endl;
      return false;
}

bool target_t::net_force(const NetForce*dev)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled NetForce node." << endl;
      return false;
}

bool target_t::net_function(const NetUserFunc*net)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled NetUserFunc node." << endl;
      return false;
}

void target_t::net_probe(const NetEvProbe*net)
{
      cerr << "target (" << typeid(*this).name() << "): "
	    "Unhandled probe trigger node" << endl;
      net->dump_node(cerr, 4);
}

bool target_t::process(const NetProcTop*top)
{
      return top->statement()->emit_proc(this);
}

void target_t::proc_assign(const NetAssign*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled procedural assignment." << endl;
}

void target_t::proc_assign_nb(const NetAssignNB*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled non-blocking assignment." << endl;
}

bool target_t::proc_block(const NetBlock*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled proc_block." << endl;
      return false;
}

void target_t::proc_case(const NetCase*cur)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled case:" << endl;
      cur->dump(cerr, 6);
}

bool target_t::proc_cassign(const NetCAssign*dev)
{
	cerr << "target (" << typeid(*this).name() <<  "): ";
	cerr << dev->get_line();
	cerr << ": Target does not support procedural continuous assignment." << endl;
      return false;
}

bool target_t::proc_condit(const NetCondit*condit)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled conditional:" << endl;
      condit->dump(cerr, 6);
      return false;
}

bool target_t::proc_deassign(const NetDeassign*dev)
{
      cerr << dev->get_line() << ": internal error: "
	   << "target (" << typeid(*this).name() <<  "): "
	   << "Unhandled proc_deassign." << endl;
      return false;
}

bool target_t::proc_delay(const NetPDelay*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled proc_delay." << endl;
      return false;
}

bool target_t::proc_disable(const NetDisable*obj)
{
      cerr << obj->get_line() << ": internal error: "
	   << "target (" << typeid(*this).name() << "): "
	   << "does not support disable statements." << endl;
      return false;
}

bool target_t::proc_force(const NetForce*dev)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled proc_force." << endl;
      return false;
}

void target_t::proc_forever(const NetForever*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled proc_forever." << endl;
}

bool target_t::proc_release(const NetRelease*dev)
{
      cerr << dev->get_line() << ": internal error: "
	   << "target (" << typeid(*this).name() <<  "): "
	   << "Unhandled proc_release." << endl;
      return false;
}

void target_t::proc_repeat(const NetRepeat*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled proc_repeat." << endl;
}

bool target_t::proc_trigger(const NetEvTrig*tr)
{
      cerr << tr->get_line() << ": error: target (" << typeid(*this).name()
	   <<  "): Unhandled event trigger." << endl;
      return false;
}

void target_t::proc_stask(const NetSTask*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled proc_stask." << endl;
}

void target_t::proc_utask(const NetUTask*)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled proc_utask." << endl;
}

bool target_t::proc_wait(const NetEvWait*tr)
{
      cerr << tr->get_line() << ": error: target (" << typeid(*this).name()
	   <<  "): Unhandled event wait." << endl;
      return false;
}

void target_t::proc_while(const NetWhile*net)
{
      cerr << "target (" << typeid(*this).name() <<  "): "
	    "Unhandled while:" << endl;
      net->dump(cerr, 6);
}

int target_t::end_design(const Design*)
{
      return 0;
}

expr_scan_t::~expr_scan_t()
{
}

void expr_scan_t::expr_const(const NetEConst*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_const." << endl;
}

void expr_scan_t::expr_param(const NetEConstParam*that)
{
      expr_const(that);
}

void expr_scan_t::expr_creal(const NetECReal*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_creal." << endl;
}

void expr_scan_t::expr_rparam(const NetECRealParam*that)
{
      expr_creal(that);
}

void expr_scan_t::expr_concat(const NetEConcat*that)
{
      cerr << that->get_line() << ": expr_scan_t (" <<
	    typeid(*this).name() << "): unhandled expr_concat." << endl;
}

void expr_scan_t::expr_memory(const NetEMemory*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_memory." << endl;
}

void expr_scan_t::expr_event(const NetEEvent*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_event." << endl;
}

void expr_scan_t::expr_scope(const NetEScope*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_scope." << endl;
}

void expr_scan_t::expr_select(const NetESelect*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_select." << endl;
}

void expr_scan_t::expr_sfunc(const NetESFunc*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_sfunc." << endl;
}

void expr_scan_t::expr_signal(const NetESignal*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_signal." << endl;
}

void expr_scan_t::expr_subsignal(const NetEBitSel*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled bit select expression." << endl;
}

void expr_scan_t::expr_ternary(const NetETernary*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_ternary." << endl;
}

void expr_scan_t::expr_ufunc(const NetEUFunc*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled function call." << endl;
}

void expr_scan_t::expr_unary(const NetEUnary*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_unary." << endl;
}

void expr_scan_t::expr_variable(const NetEVariable*)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_variable." << endl;
}

void expr_scan_t::expr_binary(const NetEBinary*ex)
{
      cerr << "expr_scan_t (" << typeid(*this).name() << "): "
	    "unhandled expr_binary: " <<*ex  << endl;
}
