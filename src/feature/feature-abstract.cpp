/*
 * Copyright 2010,
 * François Bleibel,
 * Olivier Stasse,
 *
 * CNRS/AIST
 *
 * This file is part of sot-core.
 * sot-core is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * sot-core is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.  You should
 * have received a copy of the GNU Lesser General Public License along
 * with sot-core.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sot/core/feature-abstract.hh>
#include <sot/core/pool.hh>
#include "sot/core/debug.hh"
#include "sot/core/exception-feature.hh"
#include <dynamic-graph/all-commands.h>

using namespace dynamicgraph::sot;
using dynamicgraph::sot::ExceptionFeature;

const std::string 
FeatureAbstract::CLASS_NAME = "FeatureAbstract";


FeatureAbstract::
FeatureAbstract( const std::string& name ) 
  :Entity(name)
   ,selectionSIN(NULL,"sotFeatureAbstract("+name+")::input(flag)::selec")
  ,errordotSIN (NULL,"sotFeatureAbstract("+name+")::input(vector)::errordotIN" )
   ,errorSOUT( boost::bind(&FeatureAbstract::computeError,this,_1,_2),
	       selectionSIN,
	       "sotFeatureAbstract("+name+")::output(vector)::error" )
   ,jacobianSOUT( boost::bind(&FeatureAbstract::computeJacobian,this,_1,_2),
		  selectionSIN,
		  "sotFeatureAbstract("+name+")::output(matrix)::jacobian" )
   ,dimensionSOUT( boost::bind(&FeatureAbstract::getDimension,this,_1,_2),
		   selectionSIN,
		   "sotFeatureAbstract("+name+")::output(uint)::dim" )
  ,errordotSOUT (boost::bind(&FeatureAbstract::computeErrorDot,this,_1,_2),
		 selectionSIN << errordotSIN,
		 "sotFeatureAbstract("+name+")::output(vector)::errordot" )
{
  selectionSIN = true;
  signalRegistration( selectionSIN
		      <<errorSOUT<<jacobianSOUT<<dimensionSOUT );
  featureRegistration();
  initCommands();
}

void FeatureAbstract::
initCommands( void )
{
  using namespace command;
  addCommand("setReference",
	     new dynamicgraph::command::Setter<FeatureAbstract, std::string>
	     (*this, &FeatureAbstract::setReferenceByName,
	      "Give the name of the reference feature.\nInput: a string (feature name)."));
  addCommand("getReference",
	     new dynamicgraph::command::Getter<FeatureAbstract, std::string>
	     (*this, &FeatureAbstract::getReferenceByName,
	      "Get the name of the reference feature.\nOutput: a string (feature name)."));
}

void FeatureAbstract::
featureRegistration( void )
{
  PoolStorage::getInstance()->registerFeature(name,this);
}

std::ostream& FeatureAbstract::
writeGraph( std::ostream& os ) const
{
  Entity::writeGraph(os);

  if( isReferenceSet() )
    {
      const FeatureAbstract *asotFA = getReferenceAbstract();
      os << "\t\"" << asotFA->getName() << "\" -> \"" << getName() << "\""
	 << "[ color=darkseagreen4 ]" << std::endl;
    }
  else std::cout << "asotFAT : 0" << std::endl;

  return os;
}

void FeatureAbstract::
setReferenceByName( const std::string& name )
{
  setReference( &dynamicgraph::sot::PoolStorage::getInstance()->getFeature(name));
}

std::string FeatureAbstract::
getReferenceByName() const
{
  if( isReferenceSet() ) return getReferenceAbstract()->getName(); else return "none";
}

ml::Vector& FeatureAbstract::
computeErrorDot( ml::Vector& res,int time )
{ 
  const Flags &fl = selectionSIN.access(time);
  const unsigned int & dim = dimensionSOUT(time);

  unsigned int curr = 0;
  res.resize( dim );

  sotDEBUG(25) << "Dim = " << dim << std::endl;

  if( isReferenceSet () && getReferenceAbstract ()->errordotSIN.isPluged ())
    {
      const ml::Vector& errdotDes = getReferenceAbstract ()->errordotSIN(time);
      sotDEBUG(15) << "Err* = " << errdotDes;
      if( errdotDes.size()<dim )
	{ SOT_THROW ExceptionFeature( ExceptionFeature::UNCOMPATIBLE_SIZE,
					 "Error: dimension uncompatible with des->errorIN size."
					 " (while considering feature <%s>).",getName().c_str() ); }

      for( unsigned int i=0;i<errdotDes.size();++i ) if( fl(i) ) 
	if( fl(i) ) res( curr++ ) = errdotDes(i);
    }
  else
    {
      for( unsigned int i=0;i<dim;++i )
	if( fl(i) ) res( curr++ ) = 0.0;
    }
  
  return res; 
}
