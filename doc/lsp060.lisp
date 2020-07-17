(defun notes (n)
  (loop
     initially (fms:meas :time 0 :dur 4 :comp t)
     repeat n
     for tim from 0 by 1/3
     for p = 60 then (+ p (random 5) -2)
     do (fms:note :part "pno" :time tim :dur 1/3 :pitch p)))

(fms:with-score
    (:filename *filename*
     :parts '((:id "pno" :inst "piano")))
  (notes 24))
