/* === S Y N F I G ========================================================= */
/*!	\file circle.cpp
**	\brief Implementation of the "Circle" layer
**
**	$Id$
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2008 Chris Moore
**	Copyright (c) 2011 Carlos López
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

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "circle.h"
#include <synfig/string.h>
#include <synfig/time.h>
#include <synfig/context.h>
#include <synfig/paramdesc.h>
#include <synfig/renddesc.h>
#include <synfig/surface.h>
#include <synfig/value.h>
#include <synfig/valuenode.h>

#include <cmath>

#endif

using namespace synfig;
using namespace std;
using namespace etl;

/* -- G L O B A L S --------------------------------------------------------- */

SYNFIG_LAYER_INIT(Circle);
SYNFIG_LAYER_SET_NAME(Circle,"circle");
SYNFIG_LAYER_SET_LOCAL_NAME(Circle,N_("Circle"));
SYNFIG_LAYER_SET_CATEGORY(Circle,N_("Geometry"));
SYNFIG_LAYER_SET_VERSION(Circle,"0.1");
SYNFIG_LAYER_SET_CVS_ID(Circle,"$Id$");

/* -- F U N C T I O N S ----------------------------------------------------- */

Circle::Circle():
	Layer_Composite	(1.0,Color::BLEND_COMPOSITE),
	color			(Color::black()),
	origin			(0,0),
	radius			(1),
	feather			(0),
	invert			(false),
	falloff			(FALLOFF_INTERPOLATION_LINEAR)
{
	constructcache();
	Layer::Vocab voc(get_param_vocab());
	Layer::fill_static(voc);
}

bool
Circle::ImportParameters(const String &param, const ValueBase &value)
{
	IMPORT_PLUS(color, { if (color.get_a() == 0) { if (converted_blend_) {
					set_blend_method(Color::BLEND_ALPHA_OVER);
					color.set_a(1); } else transparent_color_ = true; } });
	IMPORT(radius);
	IMPORT_PLUS(feather, if(feather<0)feather=0;);
	IMPORT(invert);
	IMPORT(origin);
	IMPORT(falloff);

	IMPORT_AS(origin,"pos");

	return Layer_Composite::set_param(param,value);
}

bool
Circle::set_param(const String &param, const ValueBase &value)
{
	if(ImportParameters(param,value))
	{
		constructcache();
		return true;
	}

	return false;
}

ValueBase
Circle::get_param(const String &param)const
{
	EXPORT(color);
	EXPORT(radius);
	EXPORT(feather);
	EXPORT(invert);
	EXPORT(origin);
	EXPORT(falloff);

	EXPORT_NAME();
	EXPORT_VERSION();

	return Layer_Composite::get_param(param);
}

Layer::Vocab
Circle::get_param_vocab()const
{
	Layer::Vocab ret(Layer_Composite::get_param_vocab());

	ret.push_back(ParamDesc("color")
		.set_local_name(_("Color"))
		.set_description(_("Fill color of the layer"))
	);
	ret.push_back(ParamDesc("radius")
		.set_local_name(_("Radius"))
		.set_origin("origin")
		.set_description(_("Radius of the circle"))
		.set_is_distance()
	);
	ret.push_back(ParamDesc("feather")
		.set_local_name(_("Feather"))
		.set_is_distance()
		.set_description(_("Amount of feather of the circle"))
	);
	ret.push_back(ParamDesc("origin")
		.set_local_name(_("Origin"))
		.set_description(_("Center of the circle"))
	);
	ret.push_back(ParamDesc("invert")
		.set_local_name(_("Invert"))
		.set_description(_("Invert the circle"))
	);

	ret.push_back(ParamDesc("falloff")
		.set_local_name(_("Falloff"))
		.set_description(_("Determines the falloff function for the feather"))
		.set_hint("enum")
		.add_enum_value(FALLOFF_INTERPOLATION_LINEAR,"linear",_("Linear"))
		.add_enum_value(FALLOFF_SQUARED,"squared",_("Squared"))
		.add_enum_value(FALLOFF_SQRT,"sqrt",_("Square Root"))
		.add_enum_value(FALLOFF_SIGMOND,"sigmond",_("Sigmond"))
		.add_enum_value(FALLOFF_COSINE,"cosine",_("Cosine"))
	);

	return ret;
}

synfig::Layer::Handle
Circle::hit_check(synfig::Context context, const synfig::Point &point)const
{
	Point temp=origin-point;

	if(get_amount()==0)
		return context.hit_check(point);

	bool in_circle(temp.mag_squared() <= radius*radius);

	if(invert)
	{
		in_circle=!in_circle;
		if(in_circle && get_amount()-(feather/radius)<=0.1 && get_blend_method()!=Color::BLEND_STRAIGHT)
			in_circle=false;
	}
	else
	{
		if(get_amount()-(feather/radius)<=0.0)
			in_circle=false;
	}

	if(in_circle)
	{
		synfig::Layer::Handle tmp;
		if(get_blend_method()==Color::BLEND_BEHIND && (tmp=context.hit_check(point)))
			return tmp;
		if(Color::is_onto(get_blend_method()) && !(tmp=context.hit_check(point)))
			return 0;
		return const_cast<Circle*>(this);
	}

	return context.hit_check(point);
}

//falloff functions
Real	Circle::SqdFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	//squared proportional falloff
	return (c.outer_radius_sqd - mag_sqd) / c.diff_sqd;
}

Real	Circle::InvSqdFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	//squared proportional falloff
	return 1.0 - (c.outer_radius_sqd - mag_sqd) / c.diff_sqd;
}


Real	Circle::SqrtFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	//linear distance falloff
	Real ret = ( c.outer_radius - sqrt(mag_sqd) ) / c.double_feather;
	//then take the square root of it
	ret = sqrt(ret);
	return ret;
}

Real	Circle::InvSqrtFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	//linear distance falloff
	Real ret = ( c.outer_radius - sqrt(mag_sqd) ) / c.double_feather;
	//then take the square root of it
	ret = 1.0 - sqrt(ret);
	return ret;
}

Real	Circle::LinearFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	//linear distance falloff
	return ( c.outer_radius - sqrt(mag_sqd) ) / c.double_feather;
}

Real	Circle::InvLinearFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	return 1.0 - ( c.outer_radius - sqrt(mag_sqd) ) / c.double_feather;
	//linear distance falloff
}

Real	Circle::SigmondFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	//linear distance falloff
	Real ret = ( c.outer_radius - sqrt(mag_sqd) ) / c.double_feather;
	// inverse exponential of the linear falloff (asymptotes at 0 and 1)
	// \frac{1.0}{ 1 + e^{- \( a*10-5 \)}}
	ret = 1.0 / (1 + exp(-(ret*10-5)) );
	return ret;
}

Real	Circle::InvSigmondFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	//linear distance falloff
	Real ret = ( c.outer_radius - sqrt(mag_sqd) ) / c.double_feather;
	// inverse exponential of the linear falloff (asymptotes at 0 and 1)
	// \frac{1.0}{ 1 + e^{- \( a*10-5 \)}}
	ret = 1.0 - 1.0 / (1 + exp(-(ret*10-5)) );
	return ret;
}


Real
Circle::CosineFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	//Cosine distance falloff
	return (1.0f-cos((( c.outer_radius - sqrt(mag_sqd) ) / c.double_feather)*3.1415927))*0.5f;
}

Real
Circle::InvCosineFalloff(const Circle::CircleDataCache &c, const Real &mag_sqd)
{
	return 1.0f-(1.0f-cos((( c.outer_radius - sqrt(mag_sqd) ) / c.double_feather)*3.1415927))*0.5f;
	//Cosine distance falloff
}

void Circle::constructcache()
{
	cache.inner_radius = radius - feather;
	if(cache.inner_radius < 0)
		cache.inner_radius = 0;

	cache.outer_radius = radius + feather;

	cache.inner_radius_sqd = cache.inner_radius > 0 ? (radius-feather)*(radius-feather) : 0;
	cache.outer_radius_sqd = (radius+feather)*(radius+feather);

	cache.diff_sqd = feather*feather*4.0;
	cache.double_feather = feather*2.0;

	falloff_func = GetFalloffFunc();
}

Circle::FALLOFF_FUNC *Circle::GetFalloffFunc()const
{
	switch(falloff)
	{
	case FALLOFF_SQUARED:	return invert?InvSqdFalloff:SqdFalloff;

	case FALLOFF_SQRT:		return invert?InvSqrtFalloff:SqrtFalloff;

	case FALLOFF_INTERPOLATION_LINEAR:	return invert?InvLinearFalloff:LinearFalloff;

	case FALLOFF_SIGMOND:	return invert?InvSigmondFalloff:SigmondFalloff;

	case FALLOFF_COSINE:
	default:				return invert?InvCosineFalloff:CosineFalloff;
	}
}

Color
Circle::get_color(Context context, const Point &point)const
{
	if(is_disabled() || (radius==0 && invert==false && !feather))
		return context.get_color(point);


	Point temp=origin-point;

	/*const Real inner_radius = radius-feather;
	const Real outer_radius = radius+feather;

	const Real inner_radius_sqd = inner_radius > 0 ? (radius-feather)*(radius-feather) : 0;
	const Real outer_radius_sqd = (radius+feather)*(radius+feather);

	const Real diff_radii_sqd = outer_radius_sqd - inner_radius_sqd;
	const Real double_feather = feather*2.0;*/

	/*const Real &inner_radius = cache.inner_radius;
	const Real &outer_radius = cache.outer_radius;*/

	const Real &inner_radius_sqd = cache.inner_radius_sqd;
	const Real &outer_radius_sqd = cache.outer_radius_sqd;

	/*const Real &diff_radii_sqd = cache.diff_radii_sqd;
	const Real &double_feather = cache.double_feather;*/

	const Vector::value_type mag_squared = temp.mag_squared();

	//Outside the circle, with feathering enabled
	if( mag_squared > outer_radius_sqd )
	{
		// inverted -> outside == colored in
		if(invert)
		{
			if(get_amount() == 1 && get_blend_method() == Color::BLEND_STRAIGHT)
				return color;
			else
				return Color::blend(color,context.get_color(point),get_amount(),get_blend_method());
		}
		else
			return Color::blend(Color::alpha(),context.get_color(point),get_amount(),get_blend_method());
	}

	//inside the circle's solid area (with feathering)
	else if(mag_squared <= inner_radius_sqd)
	{
		// !invert -> solid area
		if(!invert)
			if(get_amount() == 1 && get_blend_method() == Color::BLEND_STRAIGHT)
				return color;
			else
				return Color::blend(color,context.get_color(point),get_amount(),get_blend_method());
		else
			return Color::blend(Color::alpha(),context.get_color(point),get_amount(),get_blend_method());
	}

	//If we get here, the pixel is within the feathering area, and is thus subject to falloff
	else
	{
		Color::value_type alpha;

		/*switch(falloff)
		{

		case FALLOFF_SQUARED:
			//squared proportional falloff
			alpha = (outer_radius_sqd - mag_squared) / diff_radii_sqd;
			break;

		case FALLOFF_SQRT:
			//linear distance falloff
			alpha = ( outer_radius - sqrt(mag_squared) ) / double_feather;
			//then take the square root of it
			alpha = sqrt(alpha);
			break;

		case FALLOFF_INTERPOLATION_LINEAR:
			//linear distance falloff
			alpha = ( outer_radius - sqrt(mag_squared) ) / double_feather;
			break;

		case FALLOFF_SIGMOND:
		default:
			//linear distance falloff
			alpha = ( outer_radius - sqrt(mag_squared) ) / double_feather;
			// inverse exponential of the linear falloff (asymptotes at 0 and 1)
			// \frac{1.0}{ 1 + e^{- \( a*10-5 \)}}
			alpha = 1.0 / (1 + exp(-(alpha*10-5)) );
			break;
		}

		//If we're inverted, we need to invert the falloff value
		if(invert)
			alpha=1.0-alpha;*/

		alpha = falloff_func(cache,mag_squared);

		return Color::blend(color*alpha,context.get_color(point),get_amount(),get_blend_method());
	}
}

//Color NormalBlend(Color a, Color b, float amount)
//{
//	return (b-a)*amount+a;
//}


bool
Circle::accelerated_render(Context context,Surface *surface,int quality, const RendDesc &renddesc, ProgressCallback *cb)const
{
	// trivial case
	if(is_disabled() || (radius==0 && invert==false && !feather))
		return context.accelerated_render(surface,quality, renddesc, cb);

	// Another trivial case
	if(invert && radius==0 && is_solid_color())
	{
		surface->set_wh(renddesc.get_w(),renddesc.get_h());
		surface->fill(color);
		if(cb && !cb->amount_complete(10000,10000))
			return false;
		return true;
	}

	// Window Boundaries
	const Point	tl(renddesc.get_tl());
	const Point br(renddesc.get_br());
	const int	w(renddesc.get_w());
	const int	h(renddesc.get_h());

	const Real x_neg = tl[0] > br[0] ? -1 : 1;
	const Real y_neg = tl[1] > br[1] ? -1 : 1;

	// Width and Height of a pixel
	const Real pw = (br[0] - tl[0]) / w;
	const Real ph = (br[1] - tl[1]) / h;

	// Increasing the feather amount by the size of
	// a pixel will create an anti-aliased appearance
	// don't render feathering at all when quality is 10
	const Real newfeather = (quality == 10) ? 0 : feather + (abs(ph)+abs(pw))/4.0;

	//int u,v;
	int left = 	(int)	floor( (origin[0] - x_neg*(radius+newfeather) - tl[0]) / pw );
	int right = (int)	ceil( (origin[0] + x_neg*(radius+newfeather) - tl[0]) / pw );
	int top = 	(int)	floor( (origin[1] - y_neg*(radius+newfeather) - tl[1]) / ph );
	int bottom = (int)	ceil( (origin[1] + y_neg*(radius+newfeather) - tl[1]) / ph );

	//clip the rectangle bounds
	if(left < 0)
		left = 0;
	if(top < 0)
		top = 0;
	if(right >= w)
		right = w-1;
	if(bottom >= h)
		bottom = h-1;

	const Real inner_radius = radius-newfeather>0 ? radius-newfeather : 0;
	const Real outer_radius = radius+newfeather;

	const Real inner_radius_sqd = inner_radius*inner_radius;
	const Real outer_radius_sqd = outer_radius*outer_radius;

	const Real diff_radii_sqd = 4*newfeather*std::max(newfeather,radius);//4.0*radius*newfeather;
	const Real double_feather = newfeather * 2.0;

	//Compile the temporary cache for the falloff calculations
	FALLOFF_FUNC *func = GetFalloffFunc();

	const CircleDataCache cache =
	{
		inner_radius,outer_radius,
		inner_radius_sqd,outer_radius_sqd,
		diff_radii_sqd,double_feather
	};

	//info("Circle: Initialized everything");

	//let the rendering begin
	SuperCallback supercb(cb,0,9000,10000);

	//if it's a degenerate circle, do what we need to do, and then leave
	if(left >= right || top >= bottom)
	{
		if(invert)
		{
			if(get_amount() == 1 && get_blend_method() == Color::BLEND_STRAIGHT)
			{
				surface->set_wh(w,h);
				surface->fill(color);
				return true;
			}else
			{
				// Render what is behind us
				if(!context.accelerated_render(surface,quality,renddesc,&supercb))
				{
					if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Renderer Failure",__LINE__));
					return false;
				}

				Surface::alpha_pen p(surface->begin(),get_amount(),get_blend_method());

				p.set_value(color);
				p.put_block(h,w);
				return true;
			}
		}else
		{
			// Render what is behind us
			if(!context.accelerated_render(surface,quality,renddesc,&supercb))
			{
				if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Renderer Failure",__LINE__));
				return false;
			}
			return true;
		}
	}

	if( (origin[0] - tl[0])*(origin[0] - tl[0]) + (origin[1] - tl[1])*(origin[1] - tl[1]) < inner_radius_sqd
		&& (origin[0] - br[0])*(origin[0] - br[0]) + (origin[1] - br[1])*(origin[1] - br[1]) < inner_radius_sqd
		&& (origin[0] - tl[0])*(origin[0] - tl[0]) + (origin[1] - br[1])*(origin[1] - br[1]) < inner_radius_sqd
		&& (origin[0] - br[0])*(origin[0] - br[0]) + (origin[1] - tl[1])*(origin[1] - tl[1]) < inner_radius_sqd	)
	{
		if(invert)
		{
			// Render what is behind us
			if(!context.accelerated_render(surface,quality,renddesc,&supercb))
			{
				if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Renderer Failure",__LINE__));
				return false;
			}
		}else
		{
			if(get_amount() == 1 && get_blend_method() == Color::BLEND_STRAIGHT)
			{
				surface->set_wh(w,h);
				surface->fill(color);
				return true;
			}
		}
	}

	//info("Circle: Non degenerate, rasterize %c", invert);

	//we start in the middle of the left-top pixel
	Real leftf 	= (left + 0.5)*pw + tl[0];
	Real topf 	= (top + 0.5)*ph + tl[1];

	//the looping variables
	Real 		x,y;
	int			i,j;

	//Loop normally, since we are not inverted
	if(!invert)
	{
		// Render what is behind us
		if(!context.accelerated_render(surface,quality,renddesc,&supercb))
		{
			if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Renderer Failure",__LINE__));
			return false;
		}

		//make topf and leftf relative to the center of the circle
		leftf 	-= 	origin[0];
		topf 	-= 	origin[1];

		j = top;
		y = topf;

		//Loop over the valid y-values in the bounding square
		for(;j <= bottom; j++, y += ph)
		{
			i = left;
			x = leftf;

			//for each y-value, Loop over the bounding x-values in the bounding square
			for(;i <= right; i++, x += pw)
			{
				//for each pixel, figure out the distance and blend
				Real	r = x*x + y*y;

				//if in the inner circle then the full color shows through
				if(r <= inner_radius_sqd)
				{
					if(get_amount() == 1 && get_blend_method() == Color::BLEND_STRAIGHT)
						(*surface)[j][i]=color;
					else
						(*surface)[j][i]=Color::blend(color,(*surface)[j][i],get_amount(),get_blend_method());
				}
				//if it's within the outer circle then it's in the feathering range
				else if(r <= outer_radius_sqd)
				{
					/*float myamount;

					switch(falloff)
					{
					case FALLOFF_SQUARED:
						myamount = (outer_radius_sqd - r) / diff_radii_sqd;
						break;

					case FALLOFF_SQRT:
						myamount = (outer_radius - sqrt(r)) / double_feather;
						myamount = sqrt(myamount);
						break;

					case FALLOFF_INTERPOLATION_LINEAR:
						myamount = (outer_radius - sqrt(r)) / double_feather;
						break;

					case FALLOFF_SIGMOND:
					default:
						myamount = (outer_radius - sqrt(r)) / double_feather;
						myamount = 1.0 / ( 1 + exp(-(myamount*10 - 5)) );
						break;
					}*/

					Real	myamount = func(cache,r);

					//if(myamount<0.0)myamount=0.0;
					//if(myamount>1.0)myamount=1.0;
					myamount *= get_amount();
					(*surface)[j][i] = Color::blend(color,(*surface)[j][i],myamount,get_blend_method());
				}
			}
		}
	}
	else
	{
		Surface background;
		RendDesc desc(renddesc);
		desc.set_flags(0);

		int offset_x=0,offset_y=0;

		//fill the surface with the background color initially
		surface->set_wh(w,h);
		surface->fill(color);

		//then render the background to an alternate surface
		if(get_amount() == 1 && get_blend_method() == Color::BLEND_STRAIGHT)
		{
			offset_x = left;
			offset_y = top;

			//if there is no background showing through we are done
			if(right < left || bottom < top)
				return true;

			desc.set_subwindow(left,top,right-left+1,bottom-top+1);

			// Render what is behind us
			if(!context.accelerated_render(&background,quality,desc,&supercb))
			{
				if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Renderer Failure",__LINE__));
				return false;
			}
		}
		else
		{
			left = 0;
			right = w-1;
			top = 0;
			bottom = h-1;

			leftf = /*0.5*pw +*/ tl[0];
			topf = /*0.5*ph +*/ tl[1];

			// Render what is behind us
			if(!context.accelerated_render(&background,quality,renddesc,&supercb))
			{
				if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Renderer Failure",__LINE__));
				return false;
			}
		}

		topf -= origin[1];
		leftf-= origin[0];

		j = top;
		y = topf;

		for(;j <= bottom; j++, y+=ph)
		{
			i = left;
			x = leftf;

			for(;i <= right; i++, x+=pw)
			{
				Vector::value_type r = x*x + y*y;

				if(r < inner_radius_sqd)
				{
					(*surface)[j][i] = background[j-offset_y][i-offset_x];
				}
				else if(r < outer_radius_sqd)
				{
					/*float amount;

					switch(falloff)
					{
					case FALLOFF_SQUARED:
						amount = (r - inner_radius_sqd) / diff_radii_sqd;
						break;
					case FALLOFF_INTERPOLATION_LINEAR:
						amount = (sqrt(r) - inner_radius) / double_feather;
						break;
					case FALLOFF_SQRT:
						amount = (outer_radius - sqrt(r)) / double_feather;
						amount = 1.0 - sqrt(amount);
						break;
					case FALLOFF_SIGMOND:
					default:
						amount = (outer_radius - sqrt(r)) / double_feather;
						amount = 1.0 - ( 1.0/( 1 + exp(-(amount*10-5)) ) );
						break;
					}*/

					Real amount = func(cache,r);

					if(amount<0.0)amount=0.0;
					if(amount>1.0)amount=1.0;

					amount*=get_amount();

					(*surface)[j][i]=Color::blend(color,background[j-offset_y][i-offset_x],amount,get_blend_method());
				}else if(get_amount() != 1 || get_blend_method() != Color::BLEND_STRAIGHT)
				{
					(*surface)[j][i]=Color::blend(color,background[j][i],get_amount(),get_blend_method());
				}
			}
		}
    }

	// Mark our progress as finished
	if(cb && !cb->amount_complete(10000,10000))
		return false;

	return true;
}

///////////

bool
Circle::accelerated_cairorender(Context context,cairo_surface_t *surface,int quality, const RendDesc &renddesc, ProgressCallback *cb)const
{
	// trivial case
	if(is_disabled() || (radius==0 && invert==false && !feather))
		return context.accelerated_cairorender(surface,quality, renddesc, cb);

	// Grab the rgba values
	const float r(color.get_r());
	const float g(color.get_g());
	const float b(color.get_b());
	const float a(color.get_a());
	
	// Another trivial case
	if(invert && radius==0 && is_solid_color())
	{
		cairo_t* cr=cairo_create(surface);
		cairo_set_source_rgba(cr, r, g, b, a); // a=1.0
		cairo_paint(cr);
		cairo_restore(cr);
		cairo_destroy(cr);
		return true;
	}
	
	// Window Boundaries
	const Point	tl(renddesc.get_tl());
	const Point br(renddesc.get_br());
	const int	w(renddesc.get_w());
	const int	h(renddesc.get_h());
		
	// Width and Height of a pixel
	const Real pw = (br[0] - tl[0]) / w;
	const Real ph = (br[1] - tl[1]) / h;
	
	// True if circle is degenerated (in_radius <0)
	bool degenerated(false);
	
	// Don't render feathering at all when quality is 10
	// Cairo will take care of the anti-aliased appaerance
	const Real newfeather = (quality == 10) ? 0 : feather;

	const Real in_radius=radius-newfeather>0?radius-newfeather:0;
	const Real out_radius=radius+newfeather;
	const Point
			out_bl(origin-Point(out_radius, out_radius)),
			out_tr(origin+Point(out_radius, out_radius)),
			in_bl(origin-Point(1.0, 1.0).norm()*in_radius),
			in_tr(origin+Point(1.0, 1.0).norm()*in_radius);
	
	// if tr and bl are swaped then the circle is degenerated
	if(out_bl[0] >= out_tr[0] || out_bl[1]>=out_tr[1]) degenerated=true;
	
	cairo_t* cr=cairo_create(surface);
	// This is a rectangle with the same dimensions of the outer circle
	const Rect shape_out(out_bl, out_tr);
	const Rect shape_in(in_bl, in_tr);
	// This is a rectangle with the same dimensions of the canvas
	const Rect dest(tl, br);
	Rect inter_out(dest);
	Rect inter_in(dest);
	// inter holds the intersection rectangle of canvas and circle.
	inter_out&=shape_out;
	inter_in&=shape_in;
	
	Point inter_out_min(inter_out.get_min());
	Point inter_out_max(inter_out.get_max());
	Point inter_in_min(inter_out.get_min());
	Point inter_in_max(inter_out.get_max());

	//let the rendering begin
	SuperCallback supercb(cb,0,9000,10000);
	if(!newfeather)
	{
		if(invert)
		{
			if(
			   is_solid_color()
			   ||
			   (get_blend_method() == Color::BLEND_COMPOSITE &&
			   get_amount() == 1.0f &&
			   color.get_a() == 1.0f)
			   )
			{
				// Fill the surface with the color intially
				cairo_save(cr);
				cairo_set_source_rgba(cr, r, g, b, a); // a=1.0
				cairo_paint(cr);
				cairo_restore(cr);
				
				// Check for the case where there is nothing else to render
				if (degenerated)
				{
					cairo_destroy(cr);
					return true;
				}
				// Now clear a hole on the surface with out_radius
				cairo_save(cr);
				// This is the scale and translation values
				double tx(-tl[0]/pw);
				double ty(-tl[1]/ph);
				double sx(1/pw);
				double sy(1/ph);
				
				cairo_translate(cr, tx , ty);
				cairo_scale(cr, sx, sy);
				cairo_arc(cr, origin[0], origin[1], out_radius, 0., 2*M_PI);
				cairo_clip(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
				cairo_paint(cr);
				cairo_restore(cr);
				if(!is_solid_color())  // this means that it is just opaque.
				{
					// Since it is opaque it means that we don't
					// need to render the context beyond the outer radius
					// So first, modify the Render Description to render on
					// the intersection only.
					RendDesc desc(renddesc);
					//desc.set_flags(0);
					// this will modify the w and h values in pixels.
					desc.set_tl(Point(inter_out_min[0], inter_out_max[1]));
					desc.set_br(Point(inter_out_max[0], inter_out_min[1]));
					// create a new similar surface with the wxh dimensions
					cairo_surface_t* subimage=cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, desc.get_w(), desc.get_h());
					// Render what is behind us
					if(!context.accelerated_cairorender(subimage,quality,desc,cb))
					{
						if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Cairo Renderer Failure",__LINE__));
						{
							cairo_surface_destroy(subimage);
							cairo_destroy(cr);
							return false;
						}
					}
					// Now, place the rendered behind the inverted circle.
					// remember that the circle has Composite blend method at
					// this point
					double width (inter_out_max[0]-inter_out_min[0]);
					double height(inter_out_max[1]-inter_out_min[1]);
					// This is the scale and translation values
					double tx(-tl[0]/pw);
					double ty(-tl[1]/ph);
					double sx(1/pw);
					double sy(1/ph);
					
					cairo_save(cr);
					cairo_set_source_surface(cr, subimage, (inter_out_min[0]-tl[0])/pw, (inter_out_max[1]-tl[1])/ph);
					cairo_translate(cr, tx , ty);
					cairo_scale(cr, sx, sy);
					cairo_rectangle(cr, inter_out_min[0], inter_out_min[1], width, height);
					cairo_clip(cr);
					cairo_set_operator(cr, CAIRO_OPERATOR_DEST_OVER);
					cairo_paint(cr);
					cairo_restore(cr);
				}
				cairo_destroy(cr);
				return true;
			}
			else // inverted but semitransparent
			{
				// Initially render what's behind us
				if(!context.accelerated_cairorender(surface,quality,renddesc,cb))
				{
					if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Cairo Renderer Failure",__LINE__));
					cairo_destroy(cr);
					return false;
				}
				// Now let's render the inverted circle in other surface
				// Initially I'll fill it completely with the alpha color
				// Create a similar image with the same dimensions than the current renddesc
				cairo_surface_t* subimage=cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, renddesc.get_w(), renddesc.get_h());
				cairo_t* subcr=cairo_create(subimage);
				cairo_set_source_rgba(subcr, r, g, b, a);
				cairo_paint(subcr);
				// now remove the area of the intersection circle
				cairo_save(subcr);
				// This is the scale and translation values
				double tx(-tl[0]/pw);
				double ty(-tl[1]/ph);
				double sx(1/pw);
				double sy(1/ph);
				
				cairo_translate(subcr, tx , ty);
				cairo_scale(subcr, sx, sy);
				cairo_arc(subcr, origin[0], origin[1], out_radius, 0., 2*M_PI);
				cairo_clip(subcr);
				cairo_set_operator(subcr, CAIRO_OPERATOR_CLEAR);
				cairo_paint(subcr);
				cairo_restore(subcr);
				// now let's paint the inverted circle with the hole on the rendered context
				cairo_save(cr);
				cairo_set_source_surface(cr, subimage, 0, 0);
				cairo_set_operator(cr, CAIRO_OPERATOR_OVER); // TODO this has to be the real operator
				cairo_paint_with_alpha(cr, get_amount());
				cairo_restore(cr);
				cairo_surface_destroy(subimage);
				cairo_destroy(subcr);
				cairo_destroy(cr);
				return true;
			}
			
		}
		else // not inverted
		{
		// check if the circle covers completely the canvas
		// and it is full opaque
			if(
			   (
			   is_solid_color()
			   ||
			   (get_blend_method() == Color::BLEND_COMPOSITE &&
				get_amount() == 1.0f &&
				color.get_a() == 1.0f)
			   )
			   &&
			   inter_in==dest
			  )
				{
					// Fill the surface with the color
					cairo_save(cr);
					cairo_set_source_rgba(cr, r, g, b, a); // a=1.0
					cairo_paint(cr);
					cairo_restore(cr);
					cairo_destroy(cr);
					// Mark our progress as finished
					if(cb && !cb->amount_complete(10000,10000))
						return false;
					return true;
				}
			else // need to render the background first
			   {
				   // Initially render what's behind us
				   if(!context.accelerated_cairorender(surface,quality,renddesc,cb))
				   {
					   if(cb)cb->error(strprintf(__FILE__"%d: Accelerated Cairo Renderer Failure",__LINE__));
					   cairo_destroy(cr);
					   return false;
				   }
				   // Now draw the circle with the out_radius
				   cairo_save(cr);
				   // This is the scale and translation values
				   double tx(-tl[0]/pw);
				   double ty(-tl[1]/ph);
				   double sx(1/pw);
				   double sy(1/ph);
				   
				   cairo_translate(cr, tx , ty);
				   cairo_scale(cr, sx, sy);
				   cairo_arc(cr, origin[0], origin[1], out_radius, 0., 2*M_PI);
				   cairo_clip(cr);
				   cairo_set_operator(cr, CAIRO_OPERATOR_OVER); // TODO: this has to be the real operator
				   cairo_set_source_rgba(cr, r, g, b, a); // a=1.0
				   cairo_paint(cr);
				   cairo_restore(cr);
				   cairo_destroy(cr);
				   return true;
   			   }
		}
	}
	else // feathered circle
	{
		
	}
	cairo_destroy(cr);
	return true;
}


///////////


Rect
Circle::get_bounding_rect()const
{
	if(invert)
		return Rect::full_plane();

	Rect bounds(
		origin[0]+(radius+feather),
		origin[1]+(radius+feather),
		origin[0]-(radius+feather),
		origin[1]-(radius+feather)
	);

	return bounds;
}

Rect
Circle::get_full_bounding_rect(Context context)const
{
	if(invert)
	{
		if(is_solid_color() && color.get_a()==0)
		{
			Rect bounds(
				origin[0]+(radius+feather),
				origin[1]+(radius+feather),
				origin[0]-(radius+feather),
				origin[1]-(radius+feather)
			);
			return bounds & context.get_full_bounding_rect();
		}
		return Rect::full_plane();
	}

	return Layer_Composite::get_full_bounding_rect(context);
}
