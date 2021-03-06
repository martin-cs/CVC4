; REQUIRES: cryptominisat
; REQUIRES: drat2er
; COMMAND-LINE: --bitblast=eager --bv-sat-solver=cryptominisat --bv-proof-format=lrat --check-proofs --no-check-unsat-cores
; COMMAND-LINE: --bitblast=eager --bv-sat-solver=cryptominisat --bv-proof-format=drat --check-proofs --no-check-unsat-cores
; COMMAND-LINE: --bitblast=eager --bv-sat-solver=cryptominisat --bv-proof-format=er --check-proofs --no-check-unsat-cores
; EXPECT: unsat
(benchmark slice
  :status unsat
  :logic QF_BV
  :extrafuns ((x BitVec[8]))
  :extrafuns ((y BitVec[8]))
  :extrafuns ((z1 BitVec[4]))
  :extrafuns ((z2 BitVec[4]))
  :assumption (= x bv85[8])
  :assumption (= y bv170[8])
  :assumption (= z1 (concat (concat (concat (extract[0:0] x) (extract[2:2] x)) (extract[4:4] x)) (extract[6:6] x)))
  :assumption (= z2 (concat (concat (concat (extract[7:7] y) (extract[5:5] y)) (extract[3:3] y)) (extract[1:1] y)))
  :formula (not (= z1 z2))
)
