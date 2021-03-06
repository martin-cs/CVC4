/*********************                                                        */
/*! \file bv_ackermann.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Aina Niemetz, Yoni Zohar
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Ackermannization preprocessing pass.
 **
 ** This implements the Ackermannization preprocessing pass, which enables
 ** very limited theory combination support for eager bit-blasting via
 ** Ackermannization. It reduces constraints over the combination of the
 ** theories of fixed-size bit-vectors and uninterpreted functions as
 ** described in
 **   Liana Hadarean, An Efficient and Trustworthy Theory Solver for
 **   Bit-vectors in Satisfiability Modulo Theories.
￼**   https://cs.nyu.edu/media/publications/hadarean_liana.pdf
 **/

#include "cvc4_private.h"

#ifndef CVC4__PREPROCESSING__PASSES__BV_ACKERMANN_H
#define CVC4__PREPROCESSING__PASSES__BV_ACKERMANN_H

#include <unordered_map>
#include "expr/node.h"
#include "preprocessing/preprocessing_pass.h"
#include "preprocessing/preprocessing_pass_context.h"

namespace CVC4 {
namespace preprocessing {
namespace passes {

using TNodeSet = std::unordered_set<TNode, TNodeHashFunction>;
using FunctionToArgsMap =
    std::unordered_map<TNode, TNodeSet, TNodeHashFunction>;

class BVAckermann : public PreprocessingPass
{
 public:
   BVAckermann(PreprocessingPassContext* preprocContext);

 protected:
  /**
   * Apply Ackermannization as follows:
   *
   * - For each application f(X) where X = (x1, . . . , xn), introduce a fresh
   *   variable f_X and use it to replace all occurrences of f(X).
   *
   * - For each f(X) and f(Y) with X = (x1, . . . , xn) and Y = (y1, . . . , yn)
   *   occurring in the input formula, add the following lemma:
   *     (x_1 = y_1 /\ ... /\ x_n = y_n) => f_X = f_Y
   */
   PreprocessingPassResult applyInternal(
       AssertionPipeline* assertionsToPreprocess) override;

 private:
  FunctionToArgsMap d_funcToArgs;
  theory::SubstitutionMap d_funcToSkolem;
};

}  // namespace passes
}  // namespace preprocessing
}  // namespace CVC4

#endif
