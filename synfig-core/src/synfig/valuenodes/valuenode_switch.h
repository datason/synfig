/* === S Y N F I G ========================================================= */
/*!	\file valuenode_switch.h
**	\brief Header file for implementation of the "Switch" valuenode conversion.
**
**	$Id$
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2007 Chris Moore
**  Copyright (c) 2011 Carlos López
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === S T A R T =========================================================== */

#ifndef __SYNFIG_VALUENODE_SWITCH_H
#define __SYNFIG_VALUENODE_SWITCH_H

/* === H E A D E R S ======================================================= */

#include <synfig/valuenode.h>

/* === M A C R O S ========================================================= */

/* === C L A S S E S & S T R U C T S ======================================= */

namespace synfig {

class ValueNode_Switch : public LinkableValueNode
{
	ValueNode::RHandle link_off_;
	ValueNode::RHandle link_on_;
	ValueNode::RHandle switch_;
public:
	typedef etl::handle<ValueNode_Switch> Handle;
	typedef etl::handle<const ValueNode_Switch> ConstHandle;

	ValueNode_Switch(Type &x);

	ValueNode_Switch(const ValueBase &x);

//	static Handle create(Type &x);
//	static Handle create(const ValueNode::Handle &x);


	virtual ValueNode::LooseHandle get_link_vfunc(int i)const;

	virtual ValueBase operator()(Time t)const;

	virtual ~ValueNode_Switch();

	virtual String get_name()const;

	virtual String get_local_name()const;

protected:
	virtual bool set_link_vfunc(int i,ValueNode::Handle x);

	LinkableValueNode* create_new()const;

public:
	using synfig::LinkableValueNode::set_link_vfunc;
	static bool check_type(Type &type);
	static ValueNode_Switch* create(const ValueBase &x);
	virtual Vocab get_children_vocab_vfunc()const;
}; // END of class ValueNode_Switch

}; // END of namespace synfig

/* === E N D =============================================================== */

#endif
