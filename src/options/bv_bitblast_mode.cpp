/*********************                                                        */
/*! \file bv_bitblast_mode.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Liana Hadarean, Alex Ozdemir
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Bitblast modes for bit-vector solver.
 **
 ** Bitblast modes for bit-vector solver.
 **/

#include "options/bv_bitblast_mode.h"

#include <iostream>

namespace CVC4 {

std::ostream& operator<<(std::ostream& out, theory::bv::BitblastMode mode) {
  switch(mode) {
  case theory::bv::BITBLAST_MODE_LAZY:
    out << "BITBLAST_MODE_LAZY";
    break;
  case theory::bv::BITBLAST_MODE_EAGER:
    out << "BITBLAST_MODE_EAGER";
    break;
  default:
    out << "BitblastMode:UNKNOWN![" << unsigned(mode) << "]";
  }

  return out;
}

std::ostream& operator<<(std::ostream& out, theory::bv::BvSlicerMode mode) {
  switch(mode) {
  case theory::bv::BITVECTOR_SLICER_ON:
    out << "BITVECTOR_SLICER_ON";
    break;
  case theory::bv::BITVECTOR_SLICER_OFF:
    out << "BITVECTOR_SLICER_OFF";
    break;
  case theory::bv::BITVECTOR_SLICER_AUTO:
    out << "BITVECTOR_SLICER_AUTO";
    break;
  default:
    out << "BvSlicerMode:UNKNOWN![" << unsigned(mode) << "]";
  }

  return out;
}

std::ostream& operator<<(std::ostream& out, theory::bv::SatSolverMode solver) {
  switch(solver) {
  case theory::bv::SAT_SOLVER_MINISAT:
    out << "SAT_SOLVER_MINISAT"; 
    break;
  case theory::bv::SAT_SOLVER_CRYPTOMINISAT:
    out << "SAT_SOLVER_CRYPTOMINISAT"; 
    break;
  default:
    out << "SatSolverMode:UNKNOWN![" << unsigned(solver) << "]";
  }

  return out;
}

std::ostream& operator<<(std::ostream& out, theory::bv::BvProofFormat format)
{
  switch (format)
  {
    case theory::bv::BITVECTOR_PROOF_ER: out << "BITVECTOR_PROOF_ER"; break;
    case theory::bv::BITVECTOR_PROOF_DRAT: out << "BITVECTOR_PROOF_DRAT"; break;
    case theory::bv::BITVECTOR_PROOF_LRAT: out << "BITVECTOR_PROOF_LRAT"; break;
    default: out << "BvProofFormat:UNKNOWN![" << unsigned(format) << "]";
  }

  return out;
}

std::ostream& operator<<(std::ostream& out,
                         theory::bv::BvOptimizeSatProof level)
{
  switch (level)
  {
    case theory::bv::BITVECTOR_OPTIMIZE_SAT_PROOF_NONE:
      out << "BITVECTOR_OPTIMIZE_SAT_PROOF_NONE";
      break;
    case theory::bv::BITVECTOR_OPTIMIZE_SAT_PROOF_PROOF:
      out << "BITVECTOR_OPTIMIZE_SAT_PROOF_PROOF";
      break;
    case theory::bv::BITVECTOR_OPTIMIZE_SAT_PROOF_FORMULA:
      out << "BITVECTOR_OPTIMIZE_SAT_PROOF_FORMULA";
      break;
    default: out << "BvOptimizeSatProof:UNKNOWN![" << unsigned(level) << "]";
  }

  return out;
}

}/* CVC4 namespace */

