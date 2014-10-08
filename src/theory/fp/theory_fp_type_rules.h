#include "cvc4_private.h"

#ifndef __CVC4__THEORY__FP__THEORY_FP_TYPE_RULES_H
#define __CVC4__THEORY__FP__THEORY_FP_TYPE_RULES_H

namespace CVC4 {
namespace theory {
namespace fp {

class FloatingPointConstantTypeRule {
public:
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    const FloatingPoint &f = n.getConst<FloatingPoint>();
    
    if (check) {
      if (!(VALIDEXPONENTSIZE(f.t.exponent()))) {
        throw TypeCheckingExceptionPrivate(n, "constant with invalid exponent size");
      }
      if (!(VALIDSIGNIFICANDSIZE(f.t.significand()))) {
        throw TypeCheckingExceptionPrivate(n, "constant with invalid significand size");
      }
    }
    return nodeManager->mkFloatingPointType(f.t);
  }
};


class RoundingModeConstantTypeRule {
public:
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    // Nothing to check!
    return nodeManager->roundingModeType();
  }
};



class FloatingPointFPTypeRule {
public:
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    TypeNode signType = n[0].getType(check);
    TypeNode exponentType = n[1].getType(check);
    TypeNode significandType = n[2].getType(check);

    if (!signType.isBitVector() ||
	!exponentType.isBitVector() ||
	!significandType.isBitVector()) {
      throw TypeCheckingExceptionPrivate(n, "arguments to fp must be bit vectors");
    }

    unsigned signBits = signType.getBitVectorSize();
    unsigned exponentBits = exponentType.getBitVectorSize();
    unsigned significandBits = significandType.getBitVectorSize();

    if (check) {

      if (signBits != 1) {
	throw TypeCheckingExceptionPrivate(n, "sign bit vector in fp must be 1 bit long");
      } else if (!(VALIDEXPONENTSIZE(exponentBits))) {
	throw TypeCheckingExceptionPrivate(n, "exponent bit vector in fp is an invalid size");
      } else if (!(VALIDSIGNIFICANDSIZE(significandBits))) {
	throw TypeCheckingExceptionPrivate(n, "significand bit vector in fp is an invalid size");
      }
    }

    // The +1 is for the implicit hidden bit
    return nodeManager->mkFloatingPointType(exponentBits,significandBits + 1);
  }

};


class FloatingPointTestTypeRule {
public:
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    if (check) {
      TypeNode firstOperand = n[0].getType(check);

      if (!firstOperand.isFloatingPoint()) {
	throw TypeCheckingExceptionPrivate(n, "floating-point test applied to a non floating-point sort");
      }

      size_t children = n.getNumChildren();
      for (size_t i = 1; i < children; ++i) {
	if (!(n[i].getType(check) == firstOperand)) {
	  throw TypeCheckingExceptionPrivate(n, "floating-point test applied to mixed sorts");
	}
      }
    }

    return nodeManager->booleanType();
  }
};


class FloatingPointOperationTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    TypeNode firstOperand = n[0].getType(check);

    if (check) {
      if (!firstOperand.isFloatingPoint()) {
	throw TypeCheckingExceptionPrivate(n, "floating-point operation applied to a non floating-point sort");
      }

      size_t children = n.getNumChildren();
      for (size_t i = 1; i < children; ++i) {
	if (!(n[i].getType(check) == firstOperand)) {
	  throw TypeCheckingExceptionPrivate(n, "floating-point test applied to mixed sorts");
	}
      }
    }

    return firstOperand;
  }
};


class FloatingPointRoundingOperationTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    if (check) {
      TypeNode roundingModeType = n[0].getType(check);

      if (!roundingModeType.isRoundingMode()) {
	throw TypeCheckingExceptionPrivate(n, "first argument must be a rounding mode");
      }
    }


    TypeNode firstOperand = n[1].getType(check);

    if (check) {
      if (!firstOperand.isFloatingPoint()) {
	throw TypeCheckingExceptionPrivate(n, "floating-point operation applied to a non floating-point sort");
      }

      size_t children = n.getNumChildren();
      for (size_t i = 2; i < children; ++i) {
	if (!(n[i].getType(check) == firstOperand)) {
	  throw TypeCheckingExceptionPrivate(n, "floating-point test applied to mixed sorts");
	}
      }
    }

    return firstOperand;
  }
};


class FloatingPointToFPIEEEBitVectorTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    FloatingPointToFPIEEEBitVector info = n.getOperator().getConst<FloatingPointToFPIEEEBitVector>();

    if (check) {
      TypeNode operandType = n[0].getType(check);
      
      if (!(operandType.isBitVector())) {
	throw TypeCheckingExceptionPrivate(n, "conversion to floating-point from bit vector used with sort other than bit vector");
      } else if (!(operandType.getBitVectorSize() == info.t.exponent() + info.t.significand())) {
	throw TypeCheckingExceptionPrivate(n, "conversion to floating-point from bit vector used with bit vector length that does not match floating point parameters");
      }
    }

    return nodeManager->mkFloatingPointType(info.t);
  }
};


class FloatingPointToFPFloatingPointTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    FloatingPointToFPFloatingPoint info = n.getOperator().getConst<FloatingPointToFPFloatingPoint>();

    if (check) {
      TypeNode roundingModeType = n[0].getType(check);

      if (!roundingModeType.isRoundingMode()) {
	throw TypeCheckingExceptionPrivate(n, "first argument must be a rounding mode");
      }


      TypeNode operandType = n[1].getType(check);
      
      if (!(operandType.isFloatingPoint())) {
	throw TypeCheckingExceptionPrivate(n, "conversion to floating-point from floating-point used with sort other than floating-point");
      }
    }

    return nodeManager->mkFloatingPointType(info.t);
  }
};


class FloatingPointToFPRealTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    FloatingPointToFPReal info = n.getOperator().getConst<FloatingPointToFPReal>();

    if (check) {
      TypeNode roundingModeType = n[0].getType(check);

      if (!roundingModeType.isRoundingMode()) {
	throw TypeCheckingExceptionPrivate(n, "first argument must be a rounding mode");
      }


      TypeNode operandType = n[1].getType(check);
      
      if (!(operandType.isReal())) {
	throw TypeCheckingExceptionPrivate(n, "conversion to floating-point from real used with sort other than real");
      }
    }

    return nodeManager->mkFloatingPointType(info.t);
  }
};


class FloatingPointToFPSignedBitVectorTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    FloatingPointToFPSignedBitVector info = n.getOperator().getConst<FloatingPointToFPSignedBitVector>();

    if (check) {
      TypeNode roundingModeType = n[0].getType(check);

      if (!roundingModeType.isRoundingMode()) {
	throw TypeCheckingExceptionPrivate(n, "first argument must be a rounding mode");
      }


      TypeNode operandType = n[1].getType(check);
      
      if (!(operandType.isBitVector())) {
	throw TypeCheckingExceptionPrivate(n, "conversion to floating-point from signed bit vector used with sort other than bit vector");
      }
    }

    return nodeManager->mkFloatingPointType(info.t);
  }
};


class FloatingPointToFPUnsignedBitVectorTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    FloatingPointToFPUnsignedBitVector info = n.getOperator().getConst<FloatingPointToFPUnsignedBitVector>();

    if (check) {
      TypeNode roundingModeType = n[0].getType(check);

      if (!roundingModeType.isRoundingMode()) {
	throw TypeCheckingExceptionPrivate(n, "first argument must be a rounding mode");
      }


      TypeNode operandType = n[1].getType(check);
      
      if (!(operandType.isBitVector())) {
	throw TypeCheckingExceptionPrivate(n, "conversion to floating-point from unsigned bit vector used with sort other than bit vector");
      }
    }

    return nodeManager->mkFloatingPointType(info.t);
  }
};


class FloatingPointToUBVTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    FloatingPointToUBV info = n.getOperator().getConst<FloatingPointToUBV>();

    if (check) {
      TypeNode roundingModeType = n[0].getType(check);

      if (!roundingModeType.isRoundingMode()) {
	throw TypeCheckingExceptionPrivate(n, "first argument must be a rounding mode");
      }


      TypeNode operandType = n[1].getType(check);
      
      if (!(operandType.isFloatingPoint())) {
	throw TypeCheckingExceptionPrivate(n, "conversion to unsigned bit vector used with a sort other than floating-point");
      }
    }

    return nodeManager->mkBitVectorType(info.bvs);
  }
};


class FloatingPointToSBVTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    FloatingPointToSBV info = n.getOperator().getConst<FloatingPointToSBV>();

    if (check) {
      TypeNode roundingModeType = n[0].getType(check);

      if (!roundingModeType.isRoundingMode()) {
	throw TypeCheckingExceptionPrivate(n, "first argument must be a rounding mode");
      }


      TypeNode operandType = n[1].getType(check);
      
      if (!(operandType.isFloatingPoint())) {
	throw TypeCheckingExceptionPrivate(n, "conversion to signed bit vector used with a sort other than floating-point");
      }
    }

    return nodeManager->mkBitVectorType(info.bvs);
  }
};



class FloatingPointToRealTypeRule {
public :
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n, bool check)
      throw (TypeCheckingExceptionPrivate, AssertionException) {

    FloatingPointToReal info = n.getOperator().getConst<FloatingPointToReal>();
    TypeNode returnType = nodeManager->mkFloatingPointType(info.t);

    if (check) {
      TypeNode roundingModeType = n[0].getType(check);

      if (!roundingModeType.isRoundingMode()) {
	throw TypeCheckingExceptionPrivate(n, "first argument must be a rounding mode");
      }


      TypeNode operandType = n[1].getType(check);
      
      if (!(operandType.isFloatingPoint())) {
	throw TypeCheckingExceptionPrivate(n, "conversion to real used with a sort other than floating-point");
      }

      if (!(operandType == returnType)) {
	throw TypeCheckingExceptionPrivate(n, "operand type does not match parameter type");
      }
    }

    return returnType;
  }
};


#if 0
/*** Should be unnecessary as all operators and constants have type rules ***/
class FpTypeRule {
public:

  /**
   * Compute the type for (and optionally typecheck) a term belonging
   * to the theory of fp.
   *
   * @param check if true, the node's type should be checked as well
   * as computed.
   */
  inline static TypeNode computeType(NodeManager* nodeManager, TNode n,
                                     bool check)
    throw (TypeCheckingExceptionPrivate) {

    // TODO: implement me!
    Unimplemented();

  }

};/* class FpTypeRule */
#endif

}/* CVC4::theory::fp namespace */
}/* CVC4::theory namespace */
}/* CVC4 namespace */

#endif /* __CVC4__THEORY__FP__THEORY_FP_TYPE_RULES_H */
