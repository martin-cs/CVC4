/*
** Copyright (C) 2018 Martin Brain
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
** cvc4_symbolic.h
**
** Martin Brain
** martin.brain@cs.ox.ac.uk
** 03/06/14
**
** A back-end for symfpu that builds CVC4 nodes rather than executing the
** code directly.  This allows the symfpu code to be used to generate 
** encodings of floating-point operations.
**
*/

#include <stdint.h>

// Symfpu headers
#include "symfpu/utils/properties.h"
#include "symfpu/utils/numberOfRoundingModes.h"
#include "symfpu/core/ite.h"
#include "symfpu/core/nondet.h"

//#include "util/floatingpoint.h"
//#include "symfpu/baseTypes/cvc4_literal.h"


// CVC4 headers
#include "base/cvc4_assert.h"
#include "expr/node.h"
#include "expr/type.h"
#include "expr/node_builder.h"
#include "util/bitvector.h"

#ifdef SYMBOLIC_EVAL
// This allows debugging of the CVC4 symbolic back-end.
// By enabling this and disabling constant folding in the rewriter,
// SMT files that have operations on constants will be evaluated
// during the encoding step, which means that the expressions
// generated by the symbolic back-end can be debugged with gdb.
#include "theory/rewriter.h"
#endif

#ifndef SYMFPU_CVC4_SYMBOLIC
#define SYMFPU_CVC4_SYMBOLIC


namespace symfpu {
  namespace cvc4_symbolic {


    typedef unsigned bitWidthType;

    // Forward declarations
    class roundingMode;
    class floatingPointTypeInfo;
    class proposition;
    template <bool T> class bitVector;
    
    // Wrap up the types into one template parameter
    class traits {
    public :
      typedef bitWidthType bwt;
      typedef roundingMode rm;
      typedef floatingPointTypeInfo fpt;
      typedef proposition prop;
      typedef bitVector< true> sbv;
      typedef bitVector<false> ubv;

      static roundingMode RNE (void);
      static roundingMode RNA (void);
      static roundingMode RTP (void);
      static roundingMode RTN (void);
      static roundingMode RTZ (void);

      inline static void precondition (const bool b) { Assert(b); return; }
      inline static void postcondition (const bool b) { Assert(b); return; }
      inline static void invariant (const bool b) { Assert(b); return; }

      // TODO : These need to check a flag for the use of redundant constraints
      // and then make their way into additionalAssertions.

      inline static void precondition(const prop &p) { return; }
      inline static void postcondition(const prop &p) { return; }
      inline static void invariant(const prop &p) { return; }
     
    };

    // To simplify the property macros
    typedef traits t;


    

    typedef ::CVC4::Node Node;
    typedef ::CVC4::TNode TNode;

    class nodeWrapper {
    protected :
      // TODO : move to inheriting from Node rather than including it
      Node node;

/* SYMBOLIC_EVAL is for debugging CVC4 symbolic back-end issues.
 * Enable this and disabling constant folding will mean that operations
 * that are input with constant args are 'folded' using the symbolic encoding
 * allowing them to be traced via GDB.
 */
#ifdef SYMBOLIC_EVAL
      nodeWrapper (const Node n) : node(::CVC4::theory::Rewriter::rewrite(n)) {}
      nodeWrapper (const nodeWrapper &old) : node(::CVC4::theory::Rewriter::rewrite(old.node)) {}
#else
      nodeWrapper (const Node n) : node(n) {}
      nodeWrapper (const nodeWrapper &old) : node(old.node) {}
#endif

    public :
      const Node getNode (void) const {
	return this->node;
      }

      Node getNode (void) {
	return this->node;
      }

    };


#if SYMFPUPROPISBOOL
    // Bool version
    class proposition : public nodeWrapper {
    protected :
      bool checkNodeType (const TNode node) {
	::CVC4::TypeNode tn = node.getType(false);
	return tn.isBoolean();
      }

      friend ite<proposition, proposition>;   // For ITE

    public : 
      proposition (const Node n) : nodeWrapper(n) { PRECONDITION(checkNodeType(node)); }        // Only used within this header so could be friend'd
      proposition (bool v) : nodeWrapper(::CVC4::NodeManager::currentNM()->mkConst(v)) { PRECONDITION(checkNodeType(node)); }
      proposition (const proposition &old) : nodeWrapper(old) { PRECONDITION(checkNodeType(node)); }
      proposition (const nonDetMarkerType &)
	: nodeWrapper(::CVC4::NodeManager::currentNM()->mkSkolem("nondet_proposition", 
								 ::CVC4::NodeManager::currentNM()->booleanType(),
								 "created by symfpu")) {  PRECONDITION(checkNodeType(node)); }

      proposition operator ! (void) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::NOT, this->node));
      }

      proposition operator && (const proposition &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::AND, this->node, op.node));
      }

      proposition operator || (const proposition &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::OR, this->node, op.node));
      }

      proposition operator == (const proposition &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::IFF, this->node, op.node));
      }

      proposition operator ^ (const proposition &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::XOR, this->node, op.node));
      }

    };
#else

    // (_ BitVec 1) version
    class proposition : public nodeWrapper {
    protected :
      bool checkNodeType (const TNode node) {
	::CVC4::TypeNode tn = node.getType(false);
	return tn.isBitVector() && tn.getBitVectorSize() == 1;
      }

      friend ite<proposition, proposition>;   // For ITE

    public : 
      proposition (const Node n) : nodeWrapper(n) { PRECONDITION(checkNodeType(node)); }        // Only used within this header so could be friend'd
      proposition (bool v) : nodeWrapper(::CVC4::NodeManager::currentNM()->mkConst(::CVC4::BitVector(1U, (v?1U:0U)))) { PRECONDITION(checkNodeType(node)); }
      proposition (const proposition &old) : nodeWrapper(old) { PRECONDITION(checkNodeType(node)); }
      proposition (const nonDetMarkerType &)
	: nodeWrapper(::CVC4::NodeManager::currentNM()->mkSkolem("nondet_proposition", 
								 ::CVC4::NodeManager::currentNM()->mkBitVectorType(1),
								 "created by symfpu")) {  PRECONDITION(checkNodeType(node)); }

      proposition operator ! (void) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_NOT, this->node));
      }

      proposition operator && (const proposition &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_AND, this->node, op.node));
      }

      proposition operator || (const proposition &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_OR, this->node, op.node));
      }

      proposition operator == (const proposition &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_COMP, this->node, op.node));
      }

      proposition operator ^ (const proposition &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_XOR, this->node, op.node));
      }

    };
#endif



    class roundingMode : public nodeWrapper {
    protected :
      bool checkNodeType (const TNode n) {
	::CVC4::TypeNode tn = n.getType(false);

	return tn.isBitVector(SYMFPU_NUMBER_OF_ROUNDING_MODES);
      }

      // TODO : make this private again
    public :
      roundingMode (const Node n) : nodeWrapper(n) {  PRECONDITION(checkNodeType(node)); }

      friend ite<proposition, roundingMode>;   // For ITE

    public :
      roundingMode (const unsigned v) : nodeWrapper(::CVC4::NodeManager::currentNM()->mkConst(::CVC4::BitVector(SYMFPU_NUMBER_OF_ROUNDING_MODES, v))) {
	PRECONDITION((v & v-1) == 0 && v != 0);   // Exactly one bit set
	PRECONDITION(checkNodeType(node));
     }
      roundingMode (const roundingMode &old) : nodeWrapper(old) {  PRECONDITION(checkNodeType(node)); }

      // Not necessarily valid on creation
      roundingMode (const nonDetMarkerType &)
	: nodeWrapper(::CVC4::NodeManager::currentNM()->mkSkolem("nondet_roundingMode", 
								 ::CVC4::NodeManager::currentNM()->mkBitVectorType(SYMFPU_NUMBER_OF_ROUNDING_MODES),
								 "created by symfpu")) {  PRECONDITION(checkNodeType(node)); }

      proposition valid (void) const {
	::CVC4::NodeManager* nm = ::CVC4::NodeManager::currentNM();
	Node zero(nm->mkConst(::CVC4::BitVector(SYMFPU_NUMBER_OF_ROUNDING_MODES, (long unsigned int)0)));

	// Is there a better encoding of this?
	#ifdef SYMFPUPROPISBOOL
	return proposition(nm->mkNode(::CVC4::kind::AND,
				      nm->mkNode(::CVC4::kind::EQUAL,
						 nm->mkNode(::CVC4::kind::BITVECTOR_AND,
							    this->node,
							    nm->mkNode(::CVC4::kind::BITVECTOR_SUB,
								       this->node,
								       nm->mkConst(::CVC4::BitVector(SYMFPU_NUMBER_OF_ROUNDING_MODES, (long unsigned int)1)))),
						 zero),
				      nm->mkNode(::CVC4::kind::NOT,
						 nm->mkNode(::CVC4::kind::EQUAL,
							    this->node,
							    zero))));
	#else
	return proposition(nm->mkNode(::CVC4::kind::BITVECTOR_AND,
				      nm->mkNode(::CVC4::kind::BITVECTOR_COMP,
						 nm->mkNode(::CVC4::kind::BITVECTOR_AND,
							    this->node,
							    nm->mkNode(::CVC4::kind::BITVECTOR_SUB,
								       this->node,
								       nm->mkConst(::CVC4::BitVector(SYMFPU_NUMBER_OF_ROUNDING_MODES, (long unsigned int)1)))),
						 zero),
				      nm->mkNode(::CVC4::kind::BITVECTOR_NOT,
						 nm->mkNode(::CVC4::kind::BITVECTOR_COMP,
							    this->node,
							    zero))));
	#endif
      }

      proposition operator == (const roundingMode &op) const {
	#ifdef SYMFPUPROPISBOOL
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::EQUAL, this->node, op.node));
	#else
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_COMP, this->node, op.node));
	#endif
      }


    };



    // Type function
    template <bool T> struct signedToLiteralType;

    template <> struct signedToLiteralType< true> {
      typedef int literalType;
    };
    template <> struct signedToLiteralType<false> {
      typedef  unsigned int literalType;
    };




    template <bool isSigned>
    class bitVector : public nodeWrapper {
    protected :
      typedef typename signedToLiteralType<isSigned>::literalType literalType;

      inline Node boolNodeToBV(Node node) const {
	Assert(node.getType().isBoolean());
	::CVC4::NodeManager *nm = ::CVC4::NodeManager::currentNM();
	return nm->mkNode(::CVC4::kind::ITE,
			  node,
			  nm->mkConst(::CVC4::BitVector(1U, 1U)),
			  nm->mkConst(::CVC4::BitVector(1U, 0U)));	
      }

      inline Node BVToBoolNode(Node node) const {
	Assert(node.getType().isBitVector());
	Assert(node.getType().getBitVectorSize() == 1);
	::CVC4::NodeManager *nm = ::CVC4::NodeManager::currentNM();
	return nm->mkNode(::CVC4::kind::EQUAL,
			  node,
			  nm->mkConst(::CVC4::BitVector(1U, 1U)));
      }

      inline Node fromProposition (Node node) const {
	#ifdef PROPSYMFPUISBOOL
	return boolNodeToBV(node);
	#else
	return node;
	#endif
      }

      inline Node toProposition (Node node) const {
	#ifdef PROPSYMFPUISBOOL
	return node;
	#else
	return boolNodeToBV(node);
	#endif
      }


      
      // TODO : make this private again
    public :

      bitVector (const Node n) : nodeWrapper(n) {  PRECONDITION(checkNodeType(node)); }

      bool checkNodeType (const TNode n) {
	::CVC4::TypeNode tn = n.getType(false);
	return tn.isBitVector();
      }


      friend bitVector<!isSigned>;    // To allow conversion between the types
      friend ite<proposition, bitVector<isSigned> >;   // For ITE


    public :
      bitVector (const bitWidthType w, const unsigned v) : nodeWrapper(::CVC4::NodeManager::currentNM()->mkConst(::CVC4::BitVector(w, v))) { PRECONDITION(checkNodeType(node)); }
      bitVector (const proposition &p) : nodeWrapper(fromProposition(p.getNode())) {}
      bitVector (const bitVector<isSigned> &old) : nodeWrapper(old) {  PRECONDITION(checkNodeType(node)); }
      bitVector (const nonDetMarkerType &, const unsigned v)
	: nodeWrapper(::CVC4::NodeManager::currentNM()->mkSkolem("nondet_bitVector", 
								 ::CVC4::NodeManager::currentNM()->mkBitVectorType(v),
								 "created by symfpu")) { PRECONDITION(checkNodeType(node)); }
      bitVector (const ::CVC4::BitVector &old) : nodeWrapper(::CVC4::NodeManager::currentNM()->mkConst(old)) { PRECONDITION(checkNodeType(node)); }

      bitWidthType getWidth (void) const {
	return this->node.getType(false).getBitVectorSize();
      }


      /*** Constant creation and test ***/
      
      static bitVector<isSigned> one (const bitWidthType &w) { return bitVector<isSigned>(w,1); }
      static bitVector<isSigned> zero (const bitWidthType &w)  { return bitVector<isSigned>(w,0); }
      static bitVector<isSigned> allOnes (const bitWidthType &w)  { return bitVector<isSigned>( ~zero(w) ); }
      
      inline proposition isAllOnes() const {return (*this == bitVector<isSigned>::allOnes(this->getWidth()));}
      inline proposition isAllZeros() const {return (*this == bitVector<isSigned>::zero(this->getWidth()));}

      static bitVector<isSigned> maxValue (const bitWidthType &w);
      static bitVector<isSigned> minValue (const bitWidthType &w);

      
      /*** Operators ***/
      inline bitVector<isSigned> operator << (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_SHL, this->node, op.node));
      }

      inline bitVector<isSigned> operator >> (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode((isSigned) ? ::CVC4::kind::BITVECTOR_ASHR : ::CVC4::kind::BITVECTOR_LSHR, this->node, op.node));
      }


      inline bitVector<isSigned> operator | (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_OR, this->node, op.node));
      }

      inline bitVector<isSigned> operator & (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_AND, this->node, op.node));
      }

      inline bitVector<isSigned> operator + (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_PLUS, this->node, op.node));
      }

      inline bitVector<isSigned> operator - (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_SUB, this->node, op.node));
      }

      inline bitVector<isSigned> operator * (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_MULT, this->node, op.node));
      }
      
      inline bitVector<isSigned> operator / (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode((isSigned) ? ::CVC4::kind::BITVECTOR_SDIV : ::CVC4::kind::BITVECTOR_UDIV_TOTAL, this->node, op.node));
      }
      
      inline bitVector<isSigned> operator % (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode((isSigned) ? ::CVC4::kind::BITVECTOR_SREM : ::CVC4::kind::BITVECTOR_UREM_TOTAL, this->node, op.node));
      }
      
      inline bitVector<isSigned> operator - (void) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_NEG, this->node));
      }

      inline bitVector<isSigned> operator ~ (void) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_NOT, this->node));
      }

      inline bitVector<isSigned> increment () const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_PLUS, this->node, one(this->getWidth()).node));
      }

      inline bitVector<isSigned> decrement () const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_SUB, this->node,  one(this->getWidth()).node));
      }

      inline bitVector<isSigned> signExtendRightShift (const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_ASHR, this->node, op.node));
      }



      /*** Modular opertaions ***/
      // No overflow checking so these are the same as other operations
      inline bitVector<isSigned> modularLeftShift (const bitVector<isSigned> &op) const {
	return *this << op;
      }
      
      inline bitVector<isSigned> modularRightShift (const bitVector<isSigned> &op) const {
	return *this >> op;
      }

      inline bitVector<isSigned> modularIncrement () const {
	return this->increment();
      }

      inline bitVector<isSigned> modularDecrement () const {
	return this->decrement();
      }

      inline bitVector<isSigned> modularAdd (const bitVector<isSigned> &op) const {
	return *this + op;
      }

      inline bitVector<isSigned> modularNegate () const {
	return -(*this);
      }




      /*** Comparisons ***/

      inline proposition operator == (const bitVector<isSigned> &op) const {
#ifdef PROPSYMFPUISBOOL
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::EQUAL, this->node, op.node));
#else
	return proposition(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_COMP, this->node, op.node));
#endif
      }

    public : 

      inline proposition operator <= (const bitVector<isSigned> &op) const {
	// TODO add kind::BITVECTOR_SLEBV and BITVECTOR_ULEBV
	return (*this < op) || (*this == op);
      }

      inline proposition operator >= (const bitVector<isSigned> &op) const {
	return (*this > op) || (*this == op);
      }

      inline proposition operator < (const bitVector<isSigned> &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode((isSigned) ? ::CVC4::kind::BITVECTOR_SLTBV : ::CVC4::kind::BITVECTOR_ULTBV, this->node, op.node));
      }

      inline proposition operator > (const bitVector<isSigned> &op) const {
	return proposition(::CVC4::NodeManager::currentNM()->mkNode((isSigned) ? ::CVC4::kind::BITVECTOR_SLTBV : ::CVC4::kind::BITVECTOR_ULTBV, op.node, this->node));
      }

      /*** Type conversion ***/
      // CVC4 nodes make no distinction between signed and unsigned, thus ...
      bitVector<true> toSigned (void) const {
	return bitVector<true>(this->node);
      }
      bitVector<false> toUnsigned (void) const {
	return bitVector<false>(this->node);
      }



      /*** Bit hacks ***/

      inline bitVector<isSigned> extend (bitWidthType extension) const;

      inline bitVector<isSigned> contract (bitWidthType reduction) const {
	PRECONDITION(this->getWidth() > reduction);

	::CVC4::NodeBuilder<> construct(::CVC4::kind::BITVECTOR_EXTRACT);
	construct << ::CVC4::NodeManager::currentNM()->mkConst< ::CVC4::BitVectorExtract>(::CVC4::BitVectorExtract((this->getWidth() - 1) - reduction, 0))
		  << this->node;
	
	return bitVector<isSigned>(construct);
      }

      inline bitVector<isSigned> resize (bitWidthType newSize) const {
	bitWidthType width = this->getWidth();

	if (newSize > width) {
	  return this->extend(newSize - width);
	} else if (newSize < width) {
	  return this->contract(width - newSize);
	} else {
	  return *this;
	}
      }

      inline bitVector<isSigned> matchWidth (const bitVector<isSigned> &op) const {
	PRECONDITION(this->getWidth() <= op.getWidth());
	return this->extend(op.getWidth() - this->getWidth());
      }


      bitVector<isSigned> append(const bitVector<isSigned> &op) const {
	return bitVector<isSigned>(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_CONCAT, this->node, op.node));
      }

      // Inclusive of end points, thus if the same, extracts just one bit
      bitVector<isSigned> extract(bitWidthType upper, bitWidthType lower) const {
	PRECONDITION(upper >= lower);

	::CVC4::NodeBuilder<> construct(::CVC4::kind::BITVECTOR_EXTRACT);
	construct << ::CVC4::NodeManager::currentNM()->mkConst< ::CVC4::BitVectorExtract>(::CVC4::BitVectorExtract(upper, lower))
		  << this->node;
	
	return bitVector<isSigned>(construct);
      }


    };


    template <>
    inline bitVector<true> bitVector<true>::extend(bitWidthType extension) const {
      ::CVC4::NodeBuilder<> construct(::CVC4::kind::BITVECTOR_SIGN_EXTEND);
      construct << ::CVC4::NodeManager::currentNM()->mkConst< ::CVC4::BitVectorSignExtend>(::CVC4::BitVectorSignExtend(extension))
		<< this->node;

      return bitVector<true>(construct);
    }

    template <>
    inline bitVector<false> bitVector<false>::extend(bitWidthType extension) const {
      ::CVC4::NodeBuilder<> construct(::CVC4::kind::BITVECTOR_ZERO_EXTEND);
      construct << ::CVC4::NodeManager::currentNM()->mkConst< ::CVC4::BitVectorZeroExtend>(::CVC4::BitVectorZeroExtend(extension))
		<< this->node;

      return bitVector<false>(construct);
    }


    class floatingPointTypeInfo {
    protected :
       // If is easier to keep it wrapped as then you can make things of this type more easily
      const ::CVC4::TypeNode type;

      friend ite<proposition, floatingPointTypeInfo>;   // For ITE

    public :
      floatingPointTypeInfo(const ::CVC4::TypeNode t) : type(t) {
	PRECONDITION(t.isFloatingPoint());
      }
      floatingPointTypeInfo(unsigned exp, unsigned sig) : type(::CVC4::NodeManager::currentNM()->mkFloatingPointType(exp,sig)) {}
      floatingPointTypeInfo(const floatingPointTypeInfo &old) : type(old.type) {}
      
      bitWidthType exponentWidth(void) const    { return this->type.getFloatingPointExponentSize(); }
      bitWidthType significandWidth(void) const { return this->type.getFloatingPointSignificandSize(); }
      
      bitWidthType packedWidth(void) const            { return this->exponentWidth() + this->significandWidth(); }
      bitWidthType packedExponentWidth(void) const    { return this->exponentWidth(); }
      bitWidthType packedSignificandWidth(void) const { return this->significandWidth() - 1; }

      ::CVC4::TypeNode getTypeNode (void) const { return type; }
    };


  };


#ifdef SYMFPUPROPISBOOL
#define CVC4SYMITEDFN(T) template <>					\
    struct ite<cvc4_symbolic::proposition, T> {				\
    static const T iteOp (const cvc4_symbolic::proposition &cond,	\
			  const T &l,					\
			  const T &r) {					\
      return T(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::ITE, cond.getNode(), l.getNode(), r.getNode())); \
    }									\
  }

#else
#define CVC4SYMITEDFN(T) template <>					\
    struct ite<cvc4_symbolic::proposition, T> {				\
    static const T iteOp (const cvc4_symbolic::proposition &_cond,	\
			  const T &_l,					\
			  const T &_r) {				\
      ::CVC4::NodeManager* nm = ::CVC4::NodeManager::currentNM();	\
      									\
      ::CVC4::Node cond = _cond.getNode();				\
      ::CVC4::Node l = _l.getNode();					\
      ::CVC4::Node r = _r.getNode();					\
									\
      /* Handle some common symfpu idioms */				\
      if (cond.isConst()) {						\
	if (cond == nm->mkConst(::CVC4::BitVector(1U, 1U))) {		\
	  return l;							\
	} else {							\
	  return r;							\
	}								\
      } else {								\
	if (l.getKind() == ::CVC4::kind::BITVECTOR_ITE) {		\
	  if (l[1] == r) {						\
	    return nm->mkNode(::CVC4::kind::BITVECTOR_ITE,		\
			      nm->mkNode(::CVC4::kind::BITVECTOR_AND,	\
					 cond,				\
					 nm->mkNode(::CVC4::kind::BITVECTOR_NOT, l[0])), \
			      l[2],					\
			      r);					\
	  } else if (l[2] == r) {					\
	    return nm->mkNode(::CVC4::kind::BITVECTOR_ITE,		\
			      nm->mkNode(::CVC4::kind::BITVECTOR_AND,	\
					 cond,				\
					 l[0]),				\
			      l[1],					\
			      r);					\
	  }								\
	  								\
	} else if (r.getKind() == ::CVC4::kind::BITVECTOR_ITE) {	\
	  if (r[1] == l) {						\
	    return nm->mkNode(::CVC4::kind::BITVECTOR_ITE,		\
			      nm->mkNode(::CVC4::kind::BITVECTOR_AND,	\
					 nm->mkNode(::CVC4::kind::BITVECTOR_NOT, cond), \
					 nm->mkNode(::CVC4::kind::BITVECTOR_NOT, r[0])), \
			      r[2],					\
			      l);					\
	  } else if (r[2] == l) {					\
	    return nm->mkNode(::CVC4::kind::BITVECTOR_ITE,		\
			      nm->mkNode(::CVC4::kind::BITVECTOR_AND,	\
					 nm->mkNode(::CVC4::kind::BITVECTOR_NOT, cond), \
					 r[0]),				\
			      r[1],					\
			      l);					\
	  }								\
	  								\
	}								\
      }									\
      return T(::CVC4::NodeManager::currentNM()->mkNode(::CVC4::kind::BITVECTOR_ITE, cond, l, r)); \
    }									\
  }

#endif

  // Can (unsurprisingly) only ITE things which contain Nodes  
  CVC4SYMITEDFN(cvc4_symbolic::traits::rm);
  CVC4SYMITEDFN(cvc4_symbolic::traits::prop);
  CVC4SYMITEDFN(cvc4_symbolic::traits::sbv);
  CVC4SYMITEDFN(cvc4_symbolic::traits::ubv);

#undef CVC4SYMITEDFN

  template <>
  cvc4_symbolic::traits::ubv orderEncode<cvc4_symbolic::traits, cvc4_symbolic::traits::ubv> (const cvc4_symbolic::traits::ubv &b);

  template <>
  stickyRightShiftResult<cvc4_symbolic::traits> stickyRightShift (const cvc4_symbolic::traits::ubv &input, const cvc4_symbolic::traits::ubv &shiftAmount);

  template <>
  void probabilityAnnotation<cvc4_symbolic::traits, cvc4_symbolic::traits::prop> (const cvc4_symbolic::traits::prop &p, const probability &pr);

  
};

#endif
