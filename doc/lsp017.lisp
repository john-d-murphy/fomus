(defun notes (n)
  (loop
     repeat n
     for tim from 0 by 1/2
     for p = 60 then (+ p (random 7) -3)
     do (fms:note :part "pno" :time tim :dur 1/2 :pitch p)))

(defun beats (n)
  (loop
     repeat n
     for tim from 0 by 4
     for m in '#1=(1/2 1/4 1/8 . #1#)
     do (fms:meas :time tim :dur 4 :beat m)))

(fms:with-score
    (:filename *filename*
     :parts '((:id "pno" :inst "piano")))
  (notes 29)
  (beats 4))
