/*********************                                                        */
/*! \file theory_fp_rewriter.cpp
 ** \verbatim
 ** Original author: Martin Brain
 ** Major contributors: 
 ** Minor contributors (to current version): 
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2013  University of Oxford
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief [[ Rewrite rules for floating point theories. ]]
 **
 ** \todo [[ Constant folding
 **          Push negations up through arithmetic operators (include max and min? maybe not due to +0/-0)
 **          classifications to normal tests (maybe)
 **          (= x (fp.neg x)) --> (isNaN x)
 **          (fp.eq x (fp.neg x)) --> (isZero x)   (previous and reorganise should be sufficient)
 **          (fp.eq x const) --> various = depending on const 
 **          (fp.abs (fp.neg x)) --> (fp.abs x)
 **          (fp.isPositive (fp.neg x)) --> (fp.isNegative x)
 **          (fp.isNegative (fp.neg x)) --> (fp.isPositive x)
 **          (fp.isPositive (fp.abs x)) --> (not (isNaN x))
 **          (fp.isNegative (fp.abs x)) --> false
 **          A -> castA --> A
 **          A -> castB -> castC  -->  A -> castC if A <= B <= C
 **          A -> castB -> castA  -->  A if A <= B
 **          promotion converts can ignore rounding mode
 **       ]]
 **/

#include "theory/fp/theory_fp_rewriter.h"

#include "util/cvc4_assert.h"

#include <algorithm>

namespace CVC4 {
namespace theory {
namespace fp {

namespace rewrite {
  /** Rewrite rules **/
  template <RewriteFunction first, RewriteFunction second>
  RewriteResponse then (TNode node, bool isPreRewrite) {
    RewriteResponse result(first(node, isPreRewrite));

    if (result.status == REWRITE_DONE) {
      return second(result.node, isPreRewrite);
    } else {
      return result;
    }
  }

  RewriteResponse notFP (TNode node, bool) {
    Unreachable("non floating-point kind (%d) in floating point rewrite?",node.getKind());
  }

  RewriteResponse identity (TNode node, bool) {
    return RewriteResponse(REWRITE_DONE, node);
  }

  RewriteResponse type (TNode node, bool) {
    Unreachable("sort kind (%d) found in expression?",node.getKind());
    return RewriteResponse(REWRITE_DONE, node);
  }

  RewriteResponse removeDoubleNegation (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_NEG);
    if (node[0].getKind() == kind::FLOATINGPOINT_NEG) {
      RewriteResponse(REWRITE_AGAIN, node[0][0]);
    }

    return RewriteResponse(REWRITE_DONE, node);
  }

  RewriteResponse convertSubtractionToAddition (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_SUB);
    Node negation = NodeManager::currentNM()->mkNode(kind::FLOATINGPOINT_NEG,node[2]);
    Node addition = NodeManager::currentNM()->mkNode(kind::FLOATINGPOINT_PLUS,node[0],node[1],negation);
    return RewriteResponse(REWRITE_DONE, addition);
  }


  /* Implies (fp.eq x x) --> (not (isNaN x))
   */

  RewriteResponse ieeeEqToEq (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_EQ);
    NodeManager *nm = NodeManager::currentNM();

    return RewriteResponse(REWRITE_DONE,
			   nm->mkNode(kind::AND,
				      nm->mkNode(kind::AND,
						 nm->mkNode(kind::NOT, nm->mkNode(kind::FLOATINGPOINT_ISNAN, node[0])),
						 nm->mkNode(kind::NOT, nm->mkNode(kind::FLOATINGPOINT_ISNAN, node[1]))),
				      nm->mkNode(kind::OR,
						 nm->mkNode(kind::EQUAL, node[0], node[1]),
						 nm->mkNode(kind::AND,
							    nm->mkNode(kind::FLOATINGPOINT_ISZ, node[0]),
							    nm->mkNode(kind::FLOATINGPOINT_ISZ, node[1])))));
  }


  RewriteResponse geqToleq (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_GEQ);
    return RewriteResponse(REWRITE_DONE,NodeManager::currentNM()->mkNode(kind::FLOATINGPOINT_LEQ,node[1],node[0]));
  }

  RewriteResponse gtTolt (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_GT);
    return RewriteResponse(REWRITE_DONE,NodeManager::currentNM()->mkNode(kind::FLOATINGPOINT_LT,node[1],node[0]));
  }

  RewriteResponse removed (TNode node, bool) {  
    Unreachable("kind (%d) should have been removed?",node.getKind());
    return RewriteResponse(REWRITE_DONE, node);
  }

  RewriteResponse variable (TNode node, bool) {  
    // We should only get floating point and rounding mode variables to rewrite.
    TypeNode tn = node.getType(true);
    Assert(tn.isFloatingPoint() || tn.isRoundingMode());
 
    // Not that we do anything with them...
    return RewriteResponse(REWRITE_DONE, node);
  }

  RewriteResponse equal (TNode node, bool isPreRewrite) {
    Assert(node.getKind() == kind::EQUAL);
  
    // We should only get equalities of floating point or rounding mode types.
    TypeNode tn = node[0].getType(true);

    Assert(tn.isFloatingPoint() || tn.isRoundingMode());
    Assert(tn == node[1].getType(true));   // Should be ensured by the typing rules

    if (node[0] == node[1]) {
      return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(true));
    } else if (!isPreRewrite && (node[0] > node[1])) {
	Node normal = NodeManager::currentNM()->mkNode(kind::EQUAL,node[1],node[0]);
	return RewriteResponse(REWRITE_DONE, normal);
    } else {
      return RewriteResponse(REWRITE_DONE, node);
    }
  }


  // Note these cannot be assumed to be symmetric for +0/-0, thus no symmetry reorder
  RewriteResponse compactMinMax (TNode node, bool isPreRewrite) {
#ifdef CVC4_ASSERTIONS
    Kind k = node.getKind();
    Assert((k == kind::FLOATINGPOINT_MIN) || (k == kind::FLOATINGPOINT_MAX));
#endif
    if (node[0] == node[1]) {
      return RewriteResponse(REWRITE_DONE, node[0]);
    } else {
      return RewriteResponse(REWRITE_DONE, node);
    }
  }


  RewriteResponse reorderFPEquality (TNode node, bool isPreRewrite) {
    Assert(node.getKind() == kind::FLOATINGPOINT_EQ);
    Assert(!isPreRewrite);    // Likely redundant in pre-rewrite

    if (node[0] > node[1]) {
      Node normal = NodeManager::currentNM()->mkNode(kind::FLOATINGPOINT_EQ,node[1],node[0]);
      return RewriteResponse(REWRITE_DONE, normal);
    } else {
      return RewriteResponse(REWRITE_DONE, node);
    } 
  }

  RewriteResponse reorderBinaryOperation (TNode node, bool isPreRewrite) {
    Kind k = node.getKind();
    Assert((k == kind::FLOATINGPOINT_PLUS) || (k == kind::FLOATINGPOINT_MULT));
    Assert(!isPreRewrite);    // Likely redundant in pre-rewrite

    if (node[1] > node[2]) {
      Node normal = NodeManager::currentNM()->mkNode(k,node[0],node[2],node[1]);
      return RewriteResponse(REWRITE_DONE, normal);
    } else {
      return RewriteResponse(REWRITE_DONE, node);
    } 
  }

  RewriteResponse reorderFMA (TNode node, bool isPreRewrite) {
    Assert(node.getKind() == kind::FLOATINGPOINT_FMA);
    Assert(!isPreRewrite);    // Likely redundant in pre-rewrite

    if (node[1] > node[2]) {
      Node normal = NodeManager::currentNM()->mkNode(kind::FLOATINGPOINT_FMA,node[0],node[2],node[1],node[3]);
      return RewriteResponse(REWRITE_DONE, normal);
    } else {
      return RewriteResponse(REWRITE_DONE, node);
    } 
  }

  RewriteResponse removeSignOperations (TNode node, bool isPreRewrite) {
    Assert(node.getKind() == kind::FLOATINGPOINT_ISN   ||
	   node.getKind() == kind::FLOATINGPOINT_ISSN  ||
	   node.getKind() == kind::FLOATINGPOINT_ISZ   ||
	   node.getKind() == kind::FLOATINGPOINT_ISINF ||
	   node.getKind() == kind::FLOATINGPOINT_ISNAN);
    Assert(node.getNumChildren() == 1);

    Kind childKind(node[0].getKind());

    if ((childKind == kind::FLOATINGPOINT_NEG) ||
	(childKind == kind::FLOATINGPOINT_ABS)) {

      Node rewritten = NodeManager::currentNM()->mkNode(node.getKind(),node[0][0]);
      return RewriteResponse(REWRITE_AGAIN, rewritten);
    } else {
      return RewriteResponse(REWRITE_DONE, node);
    } 
  }

}; /* CVC4::theory::fp::rewrite */


namespace constantFold {

  RewriteResponse convertFromRealLiteral (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_TO_FP_REAL);
#if 0
    // \todo Honour the rounding mode and work for something other than doubles!

    TNode op = node.getOperator();
    const FloatingPointToFPReal &param = op.getConst<FloatingPointToFPReal>();
    Node lit =
      NodeManager::currentNM()->mkConst(FloatingPoint(param.t.exponent(),
						      param.t.significand(),
						      node[1].getConst<Rational>().getDouble()));
    
    return RewriteResponse(REWRITE_DONE, lit);
#endif
    return RewriteResponse(REWRITE_DONE, node);
  }

  RewriteResponse convertFromIEEEBitVectorLiteral (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_TO_FP_IEEE_BITVECTOR);

    TNode op = node.getOperator();
    const FloatingPointToFPIEEEBitVector &param = op.getConst<FloatingPointToFPIEEEBitVector>();
    const BitVector &bv = node[0].getConst<BitVector>();
    
    Node lit =
      NodeManager::currentNM()->mkConst(FloatingPoint(param.t.exponent(),
						      param.t.significand(),
						      bv));
    
    return RewriteResponse(REWRITE_DONE, lit);
  }

  RewriteResponse fpLiteral (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_FP);
    
    BitVector bv(node[0].getConst<BitVector>());
    bv = bv.concat(node[1].getConst<BitVector>());
    bv = bv.concat(node[2].getConst<BitVector>());
    
    // +1 to support the hidden bit
    Node lit =
      NodeManager::currentNM()->mkConst(FloatingPoint(node[1].getConst<BitVector>().getSize(),
						      node[2].getConst<BitVector>().getSize() + 1,
						      bv));
    
    return RewriteResponse(REWRITE_DONE, lit);
  }

  RewriteResponse abs (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_ABS);
    Assert(node.getNumChildren() == 1);

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(node[0].getConst<FloatingPoint>().absolute()));
  }


  RewriteResponse neg (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_NEG);
    Assert(node.getNumChildren() == 1);

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(node[0].getConst<FloatingPoint>().negate()));
  }


  RewriteResponse plus (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_PLUS);
    Assert(node.getNumChildren() == 3);

    RoundingMode rm(node[0].getConst<RoundingMode>());
    FloatingPoint arg1(node[1].getConst<FloatingPoint>());
    FloatingPoint arg2(node[2].getConst<FloatingPoint>());
    
    Assert(arg1.t == arg2.t);
      
    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(arg1.plus(rm, arg2)));
  }

  RewriteResponse mult (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_MULT);
    Assert(node.getNumChildren() == 3);

    RoundingMode rm(node[0].getConst<RoundingMode>());
    FloatingPoint arg1(node[1].getConst<FloatingPoint>());
    FloatingPoint arg2(node[2].getConst<FloatingPoint>());
    
    Assert(arg1.t == arg2.t);
    
    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(arg1.mult(rm, arg2)));
  }


  RewriteResponse equal (TNode node, bool isPreRewrite) {
    Assert(node.getKind() == kind::EQUAL);
  
    // We should only get equalities of floating point or rounding mode types.
    TypeNode tn = node[0].getType(true);

    if (tn.isFloatingPoint()) {
      FloatingPoint arg1(node[0].getConst<FloatingPoint>());
      FloatingPoint arg2(node[1].getConst<FloatingPoint>());
    
      Assert(arg1.t == arg2.t);
    
      return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(arg1 == arg2));

    } else if (tn.isRoundingMode()) {
      RoundingMode arg1(node[0].getConst<RoundingMode>());
      RoundingMode arg2(node[1].getConst<RoundingMode>());
    
      return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(arg1 == arg2));

    } else {
      Unreachable("Equality of unknown type");
    }

    return RewriteResponse(REWRITE_DONE, node);
  }


  RewriteResponse leq (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_LEQ);
    Assert(node.getNumChildren() == 2);

    FloatingPoint arg1(node[0].getConst<FloatingPoint>());
    FloatingPoint arg2(node[1].getConst<FloatingPoint>());
    
    Assert(arg1.t == arg2.t);
    
    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(arg1 <= arg2));
  }


  RewriteResponse lt (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_LT);
    Assert(node.getNumChildren() == 2);

    FloatingPoint arg1(node[0].getConst<FloatingPoint>());
    FloatingPoint arg2(node[1].getConst<FloatingPoint>());
    
    Assert(arg1.t == arg2.t);
    
    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(arg1 < arg2));
  }


  RewriteResponse isNormal (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_ISN);
    Assert(node.getNumChildren() == 1);

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(node[0].getConst<FloatingPoint>().isNormal()));
  }

  RewriteResponse isSubnormal (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_ISSN);
    Assert(node.getNumChildren() == 1);

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(node[0].getConst<FloatingPoint>().isSubnormal()));
  }

  RewriteResponse isZero (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_ISZ);
    Assert(node.getNumChildren() == 1);

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(node[0].getConst<FloatingPoint>().isZero()));
  }

  RewriteResponse isInfinite (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_ISINF);
    Assert(node.getNumChildren() == 1);

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(node[0].getConst<FloatingPoint>().isInfinite()));
  }

  RewriteResponse isNaN (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_ISNAN);
    Assert(node.getNumChildren() == 1);

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(node[0].getConst<FloatingPoint>().isNaN()));
  }

  RewriteResponse isNegative (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_ISNEG);
    Assert(node.getNumChildren() == 1);

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(node[0].getConst<FloatingPoint>().isNegative()));
  }

  RewriteResponse isPositive (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_ISPOS);
    Assert(node.getNumChildren() == 1);

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(node[0].getConst<FloatingPoint>().isPositive()));
  }

  RewriteResponse constantConvert (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_TO_FP_FLOATINGPOINT);
    Assert(node.getNumChildren() == 2);

    RoundingMode rm(node[0].getConst<RoundingMode>());
    FloatingPoint arg1(node[1].getConst<FloatingPoint>());
    FloatingPointToFPFloatingPoint info = node.getOperator().getConst<FloatingPointToFPFloatingPoint>();

    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(arg1.convert(info.t,rm)));
  }

  RewriteResponse componentFlag (TNode node, bool) {
    Kind k = node.getKind();
   
    Assert((k == kind::FLOATINGPOINT_COMPONENT_NAN ) ||
	   (k == kind::FLOATINGPOINT_COMPONENT_INF ) ||
	   (k == kind::FLOATINGPOINT_COMPONENT_ZERO ) ||
	   (k == kind::FLOATINGPOINT_COMPONENT_SIGN ));

    FloatingPoint arg0(node[0].getConst<FloatingPoint>());
    
    bool result;
    switch (k) {
    case kind::FLOATINGPOINT_COMPONENT_NAN :
      result = arg0.getLiteral().nan;
      break;
    case kind::FLOATINGPOINT_COMPONENT_INF :
      result = arg0.getLiteral().inf;
      break;
    case kind::FLOATINGPOINT_COMPONENT_ZERO :
      result = arg0.getLiteral().zero;
      break;
    case kind::FLOATINGPOINT_COMPONENT_SIGN :
      result = arg0.getLiteral().sign;
      break;
    default :
      Unreachable("Unknown kind used in constantoldComponentFlag");
      break;
    }

    BitVector res(1U, (result) ? 1U : 0U);
    
    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(res));
  }

  RewriteResponse componentExponent (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_COMPONENT_EXPONENT);

    FloatingPoint arg0(node[0].getConst<FloatingPoint>());
    
    // \todo Add a proper interface for this sort of thing to FloatingPoint
    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst((BitVector)arg0.getLiteral().exponent));
  }

  RewriteResponse componentSignificand (TNode node, bool) {
    Assert(node.getKind() == kind::FLOATINGPOINT_COMPONENT_SIGNIFICAND);

    FloatingPoint arg0(node[0].getConst<FloatingPoint>());
    
    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst((BitVector)arg0.getLiteral().significand));
  }

  RewriteResponse roundingModeBitBlast (TNode node, bool) {
    Assert(node.getKind() == kind::ROUNDINGMODE_BITBLAST);

    RoundingMode arg0(node[0].getConst<RoundingMode>());
    BitVector value;
  
    /* \todo fix the numbering of rounding modes so this doesn't need 
     * to call symfpu at all. */
    Unimplemented("Constant fold bit-blasted rounding modes.");
#if 0
    switch (arg0) {
    case roundNearestTiesToEven :
      value = symfpuSymbolic::traits::RNE().getNode().getConst<BitVector>();
      break;
      
    case roundNearestTiesToAway :
      value = symfpuSymbolic::traits::RNA().getNode().getConst<BitVector>();
      break;
	
    case roundTowardPositive :
      value = symfpuSymbolic::traits::RTP().getNode().getConst<BitVector>();
      break;
	
    case roundTowardNegative :
      value = symfpuSymbolic::traits::RTN().getNode().getConst<BitVector>();
      break;
	
    case roundTowardZero :
      value = symfpuSymbolic::traits::RTZ().getNode().getConst<BitVector>();
      break;
	
    default :
      Unreachable("Unknown rounding mode");
      break;
    }
#endif
    return RewriteResponse(REWRITE_DONE, NodeManager::currentNM()->mkConst(value));
  }



};  /* CVC4::theory::fp::constantFold */


RewriteFunction TheoryFpRewriter::preRewriteTable[kind::LAST_KIND]; 
RewriteFunction TheoryFpRewriter::postRewriteTable[kind::LAST_KIND]; 
RewriteFunction TheoryFpRewriter::constantFoldTable[kind::LAST_KIND]; 


  /**
   * Initialize the rewriter.
   */
  void TheoryFpRewriter::init() {

    /* Set up the pre-rewrite dispatch table */
    for (unsigned i = 0; i < kind::LAST_KIND; ++i) {
      preRewriteTable[i] = rewrite::notFP;
    }

    /******** Constants ********/
    /* No rewriting possible for constants */
    preRewriteTable[kind::CONST_FLOATINGPOINT] = rewrite::identity;
    preRewriteTable[kind::CONST_ROUNDINGMODE] = rewrite::identity; 

    /******** Sorts(?) ********/
    /* These kinds should only appear in types */
    //preRewriteTable[kind::ROUNDINGMODE_TYPE] = rewrite::type;
    preRewriteTable[kind::FLOATINGPOINT_TYPE] = rewrite::type;
      
    /******** Operations ********/
    preRewriteTable[kind::FLOATINGPOINT_FP] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_ABS] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_NEG] = rewrite::removeDoubleNegation;
    preRewriteTable[kind::FLOATINGPOINT_PLUS] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_SUB] = rewrite::convertSubtractionToAddition;
    preRewriteTable[kind::FLOATINGPOINT_MULT] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_DIV] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_FMA] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_SQRT] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_REM] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_RTI] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_MIN] = rewrite::compactMinMax;
    preRewriteTable[kind::FLOATINGPOINT_MAX] = rewrite::compactMinMax;

    /******** Comparisons ********/
    preRewriteTable[kind::FLOATINGPOINT_EQ] = rewrite::ieeeEqToEq;
    preRewriteTable[kind::FLOATINGPOINT_LEQ] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_LT] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_GEQ] = rewrite::geqToleq;
    preRewriteTable[kind::FLOATINGPOINT_GT] = rewrite::gtTolt;

    /******** Classifications ********/
    preRewriteTable[kind::FLOATINGPOINT_ISN] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_ISSN] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_ISZ] = rewrite::identity;  
    preRewriteTable[kind::FLOATINGPOINT_ISINF] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_ISNAN] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_ISNEG] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_ISPOS] = rewrite::identity;

    /******** Conversions ********/
    preRewriteTable[kind::FLOATINGPOINT_TO_FP_IEEE_BITVECTOR] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_TO_FP_FLOATINGPOINT] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_TO_FP_REAL] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_TO_FP_SIGNED_BITVECTOR] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_TO_FP_UNSIGNED_BITVECTOR] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_TO_FP_GENERIC] = rewrite::removed;
    preRewriteTable[kind::FLOATINGPOINT_TO_UBV] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_TO_SBV] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_TO_REAL] = rewrite::identity;

    /******** Variables ********/
    preRewriteTable[kind::VARIABLE] = rewrite::variable;
    preRewriteTable[kind::BOUND_VARIABLE] = rewrite::variable;

    preRewriteTable[kind::EQUAL] = rewrite::equal;


    /******** Components for bit-blasting ********/
    preRewriteTable[kind::FLOATINGPOINT_COMPONENT_NAN] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_COMPONENT_INF] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_COMPONENT_ZERO] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_COMPONENT_SIGN] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_COMPONENT_EXPONENT] = rewrite::identity;
    preRewriteTable[kind::FLOATINGPOINT_COMPONENT_SIGNIFICAND] = rewrite::identity;
    preRewriteTable[kind::ROUNDINGMODE_BITBLAST] = rewrite::identity;




    /* Set up the post-rewrite dispatch table */
    for (unsigned i = 0; i < kind::LAST_KIND; ++i) {
      postRewriteTable[i] = rewrite::notFP;
    }

    /******** Constants ********/
    /* No rewriting possible for constants */
    postRewriteTable[kind::CONST_FLOATINGPOINT] = rewrite::identity;
    postRewriteTable[kind::CONST_ROUNDINGMODE] = rewrite::identity; 

    /******** Sorts(?) ********/
    /* These kinds should only appear in types */
    //postRewriteTable[kind::ROUNDINGMODE_TYPE] = rewrite::type;
    postRewriteTable[kind::FLOATINGPOINT_TYPE] = rewrite::type;
      
    /******** Operations ********/
    postRewriteTable[kind::FLOATINGPOINT_FP] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_ABS] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_NEG] = rewrite::removeDoubleNegation;
    postRewriteTable[kind::FLOATINGPOINT_PLUS] = rewrite::reorderBinaryOperation;
    postRewriteTable[kind::FLOATINGPOINT_SUB] = rewrite::removed;
    postRewriteTable[kind::FLOATINGPOINT_MULT] = rewrite::reorderBinaryOperation;
    postRewriteTable[kind::FLOATINGPOINT_DIV] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_FMA] = rewrite::reorderFMA;
    postRewriteTable[kind::FLOATINGPOINT_SQRT] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_REM] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_RTI] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_MIN] = rewrite::compactMinMax;
    postRewriteTable[kind::FLOATINGPOINT_MAX] = rewrite::compactMinMax;

    /******** Comparisons ********/
    postRewriteTable[kind::FLOATINGPOINT_EQ] = rewrite::removed;
    postRewriteTable[kind::FLOATINGPOINT_LEQ] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_LT] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_GEQ] = rewrite::removed;
    postRewriteTable[kind::FLOATINGPOINT_GT] = rewrite::removed;

    /******** Classifications ********/
    postRewriteTable[kind::FLOATINGPOINT_ISN] = rewrite::removeSignOperations;
    postRewriteTable[kind::FLOATINGPOINT_ISSN] = rewrite::removeSignOperations;
    postRewriteTable[kind::FLOATINGPOINT_ISZ] = rewrite::removeSignOperations;
    postRewriteTable[kind::FLOATINGPOINT_ISINF] = rewrite::removeSignOperations;
    postRewriteTable[kind::FLOATINGPOINT_ISNAN] = rewrite::removeSignOperations;
    postRewriteTable[kind::FLOATINGPOINT_ISNEG] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_ISPOS] = rewrite::identity;

    /******** Conversions ********/
    postRewriteTable[kind::FLOATINGPOINT_TO_FP_IEEE_BITVECTOR] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_TO_FP_FLOATINGPOINT] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_TO_FP_REAL] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_TO_FP_SIGNED_BITVECTOR] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_TO_FP_UNSIGNED_BITVECTOR] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_TO_FP_GENERIC] = rewrite::removed;
    postRewriteTable[kind::FLOATINGPOINT_TO_UBV] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_TO_SBV] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_TO_REAL] = rewrite::identity;

    /******** Variables ********/
    postRewriteTable[kind::VARIABLE] = rewrite::variable;
    postRewriteTable[kind::BOUND_VARIABLE] = rewrite::variable;

    postRewriteTable[kind::EQUAL] = rewrite::equal;


    /******** Components for bit-blasting ********/
    postRewriteTable[kind::FLOATINGPOINT_COMPONENT_NAN] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_COMPONENT_INF] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_COMPONENT_ZERO] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_COMPONENT_SIGN] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_COMPONENT_EXPONENT] = rewrite::identity;
    postRewriteTable[kind::FLOATINGPOINT_COMPONENT_SIGNIFICAND] = rewrite::identity;
    postRewriteTable[kind::ROUNDINGMODE_BITBLAST] = rewrite::identity;



    /* Set up the post-rewrite constant fold table */
    for (unsigned i = 0; i < kind::LAST_KIND; ++i) {
      constantFoldTable[i] = rewrite::notFP;
    }

    /******** Constants ********/
    /* Already folded! */
    constantFoldTable[kind::CONST_FLOATINGPOINT] = rewrite::identity;
    constantFoldTable[kind::CONST_ROUNDINGMODE] = rewrite::identity; 

    /******** Sorts(?) ********/
    /* These kinds should only appear in types */
    constantFoldTable[kind::FLOATINGPOINT_TYPE] = rewrite::type;
      
    /******** Operations ********/
    constantFoldTable[kind::FLOATINGPOINT_FP] = constantFold::fpLiteral;
    constantFoldTable[kind::FLOATINGPOINT_ABS] = constantFold::abs;
    constantFoldTable[kind::FLOATINGPOINT_NEG] = constantFold::neg;
    constantFoldTable[kind::FLOATINGPOINT_PLUS] = constantFold::plus;
    constantFoldTable[kind::FLOATINGPOINT_SUB] = rewrite::removed;
    constantFoldTable[kind::FLOATINGPOINT_MULT] = constantFold::mult;
    constantFoldTable[kind::FLOATINGPOINT_DIV] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_FMA] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_SQRT] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_REM] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_RTI] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_MIN] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_MAX] = rewrite::identity;

    /******** Comparisons ********/
    constantFoldTable[kind::FLOATINGPOINT_EQ] = rewrite::removed;
    constantFoldTable[kind::FLOATINGPOINT_LEQ] = constantFold::leq;
    constantFoldTable[kind::FLOATINGPOINT_LT] = constantFold::lt;
    constantFoldTable[kind::FLOATINGPOINT_GEQ] = rewrite::removed;
    constantFoldTable[kind::FLOATINGPOINT_GT] = rewrite::removed;

    /******** Classifications ********/
    constantFoldTable[kind::FLOATINGPOINT_ISN] = constantFold::isNormal;
    constantFoldTable[kind::FLOATINGPOINT_ISSN] = constantFold::isSubnormal;
    constantFoldTable[kind::FLOATINGPOINT_ISZ] = constantFold::isZero;
    constantFoldTable[kind::FLOATINGPOINT_ISINF] = constantFold::isInfinite;
    constantFoldTable[kind::FLOATINGPOINT_ISNAN] = constantFold::isNaN;
    constantFoldTable[kind::FLOATINGPOINT_ISNEG] = constantFold::isNegative;
    constantFoldTable[kind::FLOATINGPOINT_ISPOS] = constantFold::isPositive;

    /******** Conversions ********/
    constantFoldTable[kind::FLOATINGPOINT_TO_FP_IEEE_BITVECTOR] = constantFold::convertFromIEEEBitVectorLiteral;
    constantFoldTable[kind::FLOATINGPOINT_TO_FP_FLOATINGPOINT] = constantFold::constantConvert;
    constantFoldTable[kind::FLOATINGPOINT_TO_FP_REAL] = constantFold::convertFromRealLiteral;
    constantFoldTable[kind::FLOATINGPOINT_TO_FP_SIGNED_BITVECTOR] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_TO_FP_UNSIGNED_BITVECTOR] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_TO_FP_GENERIC] = rewrite::removed;
    constantFoldTable[kind::FLOATINGPOINT_TO_UBV] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_TO_SBV] = rewrite::identity;
    constantFoldTable[kind::FLOATINGPOINT_TO_REAL] = rewrite::identity;

    /******** Variables ********/
    constantFoldTable[kind::VARIABLE] = rewrite::variable;
    constantFoldTable[kind::BOUND_VARIABLE] = rewrite::variable;

    constantFoldTable[kind::EQUAL] = constantFold::equal;


    /******** Components for bit-blasting ********/
    constantFoldTable[kind::FLOATINGPOINT_COMPONENT_NAN] = constantFold::componentFlag;
    constantFoldTable[kind::FLOATINGPOINT_COMPONENT_INF] = constantFold::componentFlag;
    constantFoldTable[kind::FLOATINGPOINT_COMPONENT_ZERO] = constantFold::componentFlag;
    constantFoldTable[kind::FLOATINGPOINT_COMPONENT_SIGN] = constantFold::componentFlag;
    constantFoldTable[kind::FLOATINGPOINT_COMPONENT_EXPONENT] = constantFold::componentExponent;
    constantFoldTable[kind::FLOATINGPOINT_COMPONENT_SIGNIFICAND] = constantFold::componentSignificand;
    constantFoldTable[kind::ROUNDINGMODE_BITBLAST] = constantFold::roundingModeBitBlast;


  }




  /**
   * Rewrite a node into the normal form for the theory of fp
   * in pre-order (really topological order)---meaning that the
   * children may not be in the normal form.  This is an optimization
   * for theories with cancelling terms (e.g., 0 * (big-nasty-expression)
   * in arithmetic rewrites to 0 without the need to look at the big
   * nasty expression).  Since it's only an optimization, the
   * implementation here can do nothing.
   */

  RewriteResponse TheoryFpRewriter::preRewrite(TNode node) {
    Trace("fp-rewrite") << "TheoryFpRewriter::preRewrite(): " << node << std::endl;
    RewriteResponse res = preRewriteTable [node.getKind()] (node, true);
    if (res.node != node) {
      Debug("fp-rewrite") << "TheoryFpRewriter::preRewrite(): before " << node << std::endl;
      Debug("fp-rewrite") << "TheoryFpRewriter::preRewrite(): after  " << res.node << std::endl;
    }
    return res;
  }


  /**
   * Rewrite a node into the normal form for the theory of fp.
   * Called in post-order (really reverse-topological order) when
   * traversing the expression DAG during rewriting.  This is the
   * main function of the rewriter, and because of the ordering,
   * it can assume its children are all rewritten already.
   *
   * This function can return one of three rewrite response codes
   * along with the rewritten node:
   *
   *   REWRITE_DONE indicates that no more rewriting is needed.
   *   REWRITE_AGAIN means that the top-level expression should be
   *     rewritten again, but that its children are in final form.
   *   REWRITE_AGAIN_FULL means that the entire returned expression
   *     should be rewritten again (top-down with preRewrite(), then
   *     bottom-up with postRewrite()).
   *
   * Even if this function returns REWRITE_DONE, if the returned
   * expression belongs to a different theory, it will be fully
   * rewritten by that theory's rewriter.
   */

  RewriteResponse TheoryFpRewriter::postRewrite(TNode node) {
    Trace("fp-rewrite") << "TheoryFpRewriter::postRewrite(): " << node << std::endl;
    RewriteResponse res = postRewriteTable [node.getKind()] (node, false);
    if (res.node != node) {
      Debug("fp-rewrite") << "TheoryFpRewriter::postRewrite(): before " << node << std::endl;
      Debug("fp-rewrite") << "TheoryFpRewriter::postRewrite(): after  " << res.node << std::endl;
    }

    if (res.status == REWRITE_DONE) {
      bool allChildrenConst = true;
      for (Node::const_iterator i = res.node.begin();
	   i != res.node.end();
	   ++i) {

	if ((*i).getMetaKind() != kind::metakind::CONSTANT) {
	  allChildrenConst = false;
	  break;
	}
      }

      if (allChildrenConst) {
	RewriteResponse constRes = constantFoldTable [res.node.getKind()] (res.node, false);

	if (constRes.node != res.node) {
	  Debug("fp-rewrite") << "TheoryFpRewriter::postRewrite(): before constant fold " << res.node << std::endl;
	  Debug("fp-rewrite") << "TheoryFpRewriter::postRewrite(): after constant fold" << constRes.node << std::endl;
	}

	return constRes;
      }
    }

    return res;
  }


}/* CVC4::theory::fp namespace */
}/* CVC4::theory namespace */
}/* CVC4 namespace */

