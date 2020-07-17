(defun notes (st n)
  (loop
     repeat n
     for tim from st by 1/2
     for p = 60 then (+ p (random 7) -3)
     do (fms:note :part "pno" :time tim :dur 1/2 :pitch p)))

(fms:with-score
    (:filename *filename*
     :parts '((:id "pno" :inst "piano")))
  (loop
     repeat 5
     for tim = 0 then (+ tim (random 8) 1)
     do
       (fms:meas :time tim :dur 3)
       (notes tim (1+ (random 8)))))
