/*
 * Copyright (c) 2008 Stephen Williams (steve@icarus.com)
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

# include  "vvp_island.h"
# include  "compile.h"
# include  "symbols.h"
# include  "schedule.h"
# include  <list>

using namespace std;

class vvp_island_tran : public vvp_island {

    public:
      void run_island();
};

struct vvp_island_branch_tran : public vvp_island_branch {

	// Behavior. (This stuff should be moved to a derived
	// class. The members here are specific to the tran island
	// class.)
      bool run_test_enabled();
      void run_resolution();
      bool active_high;
      bool enabled_flag;
      vvp_net_t*en;
      unsigned width, part, offset;

      void clear_resolution_flags() { flags_ &= ~0x0f; }

      void mark_done(unsigned ab)       { flags_ |= 1 << ab; }
      bool test_done(unsigned ab) const { return flags_ & (1<<ab); }

      void clear_visited(unsigned ab)      { flags_ &= ~(4 << ab); }
      void mark_visited(unsigned ab)       { flags_ |= 4 << ab; }
      bool test_visited(unsigned ab) const { return flags_ & (4<<ab); }

	// Use the peek only for diagnostic purposes.
      int peek_flags() const { return flags_; }

    private:
      int flags_;
};

static inline vvp_island_branch_tran* BRANCH_TRAN(vvp_island_branch*tmp)
{
      vvp_island_branch_tran*res = dynamic_cast<vvp_island_branch_tran*>(tmp);
      assert(res);
      return res;
}

/*
 * The run_island() method is called by the scheduler to run the
 * entire island. We run the island by calling run_resolution() for
 * all the branches in the island.
*/
void vvp_island_tran::run_island()
{
	// Test to see if any of the branches are enabled. This loop
	// tests the enabled inputs for all the branches and caches
	// the results in the enabled_flag for each branch. The
	// run_test_enabled() method also clears all the processing
	// flags for the branches so that we are in a good start
	// state.
      bool runnable = false;
      for (vvp_island_branch*cur = branches_ ; cur ; cur = cur->next_branch) {
	    vvp_island_branch_tran*tmp = dynamic_cast<vvp_island_branch_tran*>(cur);
	    assert(tmp);
	    runnable |= tmp->run_test_enabled();
      }

	// Now resolve all the branches in the island.
      for (vvp_island_branch*cur = branches_ ; cur ; cur = cur->next_branch) {
	    vvp_island_branch_tran*tmp = dynamic_cast<vvp_island_branch_tran*>(cur);
	    assert(tmp);
	    tmp->run_resolution();
      }
}

bool vvp_island_branch_tran::run_test_enabled()
{
	// Clear all the flags.
      clear_resolution_flags();

      vvp_island_port*ep = en? dynamic_cast<vvp_island_port*> (en->fun) : 0;

	// If there is no ep port (no "enabled" input) then this is a
	// tran branch. Assume it is always enabled.
      if (ep == 0) {
	    enabled_flag = true;
	    return true;
      }

	// Get the input that is driving this enable.
	// SPECIAL NOTE: Try to get the input value from the
	// *outvalue* of the port. If the enable is connected to a
	// .port (instead of a .import) then there may be feedback
	// going on, and we need to be looking at the resolved input,
	// not the event input. For example:
	//
	//   tranif1 (pin, X, pin);
	//
	// In this case, when we test the value for "pin", we need to
	// look at the value that is resolved from this
	// island. Reading the outvalue will do the trick.
	//
	// If the outvalue is nil, then we know that this port is a
	// .import after all, so just read the invalue.
      enabled_flag = false;
      vvp_bit4_t enable_val;
      if (ep->outvalue.size() != 0)
	    enable_val = ep->outvalue.value(0).value();
      else if (ep->invalue.size() == 0)
	    enable_val = BIT4_Z;
      else
	    enable_val = ep->invalue.value(0).value();

      if (active_high==true && enable_val != BIT4_1)
	    return false;

      if (active_high==false && enable_val != BIT4_0)
	    return false;

      enabled_flag = true;
      return true;
}

static void island_send_value(list<vvp_branch_ptr_t>&connections, const vvp_vector8_t&val)
{
      for (list<vvp_branch_ptr_t>::iterator idx = connections.begin()
		 ; idx != connections.end() ; idx ++ ) {

	    vvp_island_branch*tmp_ptr = idx->ptr();

	    unsigned tmp_ab = idx->port();
	    island_send_value(tmp_ab? tmp_ptr->b : tmp_ptr->a, val);
      }
}

static void mark_done_flags(list<vvp_branch_ptr_t>&connections)
{
      for (list<vvp_branch_ptr_t>::iterator idx = connections.begin()
		 ; idx != connections.end() ; idx ++ ) {

	    vvp_island_branch*tmp_ptr = idx->ptr();
	    vvp_island_branch_tran*cur = dynamic_cast<vvp_island_branch_tran*>(tmp_ptr);

	    unsigned tmp_ab = idx->port();
	    cur->mark_done(tmp_ab);
      }
}

static void mark_visited_flags(list<vvp_branch_ptr_t>&connections)
{
      for (list<vvp_branch_ptr_t>::iterator idx = connections.begin()
		 ; idx != connections.end() ; idx ++ ) {

	    vvp_island_branch*tmp_ptr = idx->ptr();
	    vvp_island_branch_tran*cur = dynamic_cast<vvp_island_branch_tran*>(tmp_ptr);
	    assert(cur);

	    unsigned tmp_ab = idx->port();
	    cur->mark_visited(tmp_ab);
      }
}

static void clear_visited_flags(list<vvp_branch_ptr_t>&connections)
{
      for (list<vvp_branch_ptr_t>::iterator idx = connections.begin()
		 ; idx != connections.end() ; idx ++ ) {

	    vvp_island_branch_tran*tmp_ptr = BRANCH_TRAN(idx->ptr());

	    unsigned tmp_ab = idx->port();
	    tmp_ptr->clear_visited(tmp_ab);
      }
}

static vvp_vector8_t get_value_from_branch(vvp_branch_ptr_t cur);

static void resolve_values_from_connections(vvp_vector8_t&val,
					    list<vvp_branch_ptr_t>&connections)
{
      for (list<vvp_branch_ptr_t>::iterator idx = connections.begin()
		 ; idx != connections.end() ; idx ++ ) {
	    vvp_vector8_t tmp = get_value_from_branch(*idx);
	    if (val.size() == 0)
		  val = tmp;
	    else if (tmp.size() != 0)
		  val = resolve(val, tmp);
      }
}

static vvp_vector8_t get_value_from_branch(vvp_branch_ptr_t cur)
{
      vvp_island_branch_tran*ptr = BRANCH_TRAN(cur.ptr());
      assert(ptr);
      unsigned ab = cur.port();
      unsigned ab_other = ab^1;

	// If the branch link is disabled, return nil.
      if (ptr->enabled_flag == false)
	    return vvp_vector8_t();

      vvp_branch_ptr_t other (ptr, ab_other);

	// If the branch other side is already visited, return
	// nil. This prevents recursion loops.
      if (ptr->test_visited(ab_other))
	    return vvp_vector8_t();

	// Other side net, and port value.
      vvp_net_t*net_other = ab? ptr->a : ptr->b;
      vvp_vector8_t val_other = island_get_value(net_other);

	// recurse
      list<vvp_branch_ptr_t> connections;
      island_collect_node(connections, other);
      mark_visited_flags(connections);

      resolve_values_from_connections(val_other, connections);

	// Remove/unwind visited flags
      clear_visited_flags(connections);

      if (val_other.size() == 0)
	    return val_other;

      if (ptr->width) {
	    if (ab == 0) {
		  val_other = part_expand(val_other, ptr->width, ptr->offset);

	    } else {
		  val_other = val_other.subvalue(ptr->offset, ptr->part);

	    }
      }

      return val_other;
}

/*
 * Try to recursively push a fully resolved value back through the
 * graph. This can save many span iterations through the graph by
 * marking as done that are obviously and easily done. But it is
 * better to be conservative here.
 *
 * The connections list is filled with connections that are already
 * marked done, and the val is the resolved value. We are going to try
 * to follow branches to see if we can push the value further and mark
 * the other side done as well.
 */
static void push_value_through_branches(const vvp_vector8_t&val,
					list<vvp_branch_ptr_t>&connections)
{
      for (list<vvp_branch_ptr_t>::iterator idx = connections.begin()
		 ; idx != connections.end() ; idx ++ ) {

	    vvp_island_branch_tran*tmp_ptr = BRANCH_TRAN(idx->ptr());
	    unsigned tmp_ab = idx->port();
	    unsigned other_ab = tmp_ab^1;

	      // If other side already done, skip
	    if (tmp_ptr->test_done(other_ab))
		  continue;

	      // If link is not enabled, skip.
	    if (! tmp_ptr->enabled_flag)
		  continue;

	    vvp_net_t*other_net = other_ab? tmp_ptr->b : tmp_ptr->a;

	    if (tmp_ptr->width == 0) {
		    // There are no part selects, so we can safely
		    // Mark this end as done.
		  tmp_ptr->mark_done(other_ab);
		  island_send_value(other_net, val);

	    } else if (other_ab == 1) {
		    // The other side is a strict subset (part select)
		    // of this side, so we can mark this end as done.
		  tmp_ptr->mark_done(other_ab);
		  vvp_vector8_t tmp = val.subvalue(tmp_ptr->offset, tmp_ptr->part);
		  island_send_value(other_net, tmp);

	    } else {
		    // Otherwise, the other side is not fully
		    // specified (is a subset of the done side) so we
		    // can't take this shortcut.
	    }
      }
}

/*
 * This method resolves the value for a branch recursively. It uses
 * recursive descent to span the graph of branches, collecting values
 * that need to be resolved together.
 */
void vvp_island_branch_tran::run_resolution()
{
	// Collect all the branch endpoints that are joined to my A
	// side.
      list<vvp_branch_ptr_t> connections;
      bool processed_a_side = false;
      vvp_vector8_t val;

	// The "flags" member is a bitmask that marks whether an
	// endpoint of a branch has been visited. If flags&1, then the
	// A side has been visited. If flags&2, then the B side has
	// been visited. The flags help us avoid recursion when doing
	// spanning trees.

	// If the A side has already been completed, then skip it.
      if (! test_done(0)) {
	    processed_a_side = true;
	    vvp_branch_ptr_t a_side(this, 0);
	    island_collect_node(connections, a_side);

	      // Mark my A side as done. Do this early to prevent recursing
	      // back. All the connections that share this port are also
	      // done. Make sure their flags are set appropriately.
	    mark_done_flags(connections);

	      // Start with my branch-point value.
	    val = island_get_value(a);
	    mark_visited_flags(connections); // Mark as visited.


	      // Now scan the other sides of all the branches connected to
	      // my A side. The get_value_from_branch() will recurse as
	      // necessary to depth-first walk the graph.
	    resolve_values_from_connections(val, connections);

	      // A side is done.
	    island_send_value(connections, val);

	      // Clear the visited flags. This must be done so that other
	      // branches can read this input value.
	    clear_visited_flags(connections);

	      // Try to push the calculated value out through the
	      // branches. This is useful for A-side results because
	      // there is a high probability that the other side of
	      // all the connected branches is fully specified by this
	      // result.
	    push_value_through_branches(val, connections);
      }

	// If the B side got taken care of by above, then this branch
	// is done. Stop now.
      if (test_done(1))
	    return;

	// Repeat the above for the B side.

      connections.clear();
      island_collect_node(connections, vvp_branch_ptr_t(this, 1));
      mark_done_flags(connections);

      if (enabled_flag && processed_a_side) {
	      // If this is a connected branch, then we know from the
	      // start that we have all the bits needed to complete
	      // the B side. Even if the B side is a part select, the
	      // simple part select must be correct because the
	      // recursive resolve_values_from_connections above must
	      // of cycled back to the B side of myself when resolving
	      // the connections.
	    if (width != 0)
		  val = val.subvalue(offset, part);

      } else {

	      // If this branch is not enabled, then the B-side must
	      // be processed on its own.
	    val = island_get_value(b);
	    mark_visited_flags(connections);
	    resolve_values_from_connections(val, connections);
	    clear_visited_flags(connections);
      }

      island_send_value(connections, val);
}

void compile_island_tran(char*label)
{
      vvp_island*use_island = new vvp_island_tran;
      compile_island_base(label, use_island);
}

void compile_island_tranif(int sense, char*island, char*pa, char*pb, char*pe)
{
      vvp_island*use_island = compile_find_island(island);
      assert(use_island);
      free(island);

      vvp_island_branch_tran*br = new vvp_island_branch_tran;
      if (sense)
	    br->active_high = true;
      else
	    br->active_high = false;

      if (pe == 0) {
	    br->en = 0;
      } else {
	    br->en = use_island->find_port(pe);
	    assert(br->en);
	    free(pe);
      }

      br->width = 0;
      br->part = 0;
      br->offset = 0;

      use_island->add_branch(br, pa, pb);

      free(pa);
      free(pb);
}


void compile_island_tranvp(char*island, char*pa, char*pb,
			   unsigned wid, unsigned par, unsigned off)
{
      vvp_island*use_island = compile_find_island(island);
      assert(use_island);
      free(island);

      vvp_island_branch_tran*br = new vvp_island_branch_tran;
      br->active_high = false;
      br->en = 0;
      br->width = wid;
      br->part = par;
      br->offset = off;

      use_island->add_branch(br, pa, pb);

      free(pa);
      free(pb);
}
