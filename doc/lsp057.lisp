(defun notes (n)
  (loop
     repeat n
     for tim from 0 by 1
     for p = 60 then (+ p (random 7) -3)
     do (fms:note :part "pno" :time tim :dur 1 :pitch p)))

(fms:with-score
    (:filename *filename*
     :sets '(:beat 1/8)
     :parts '((:id "pno" :inst "piano")))
  (notes 16))
