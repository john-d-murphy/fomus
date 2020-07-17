(defun notes ()
  (loop
     for tim from 0 below 10 by 1/2
     for p in '#1=(60 61 62 63 64 65 64 63 62 61 . #1#)
     do (fms:note :part "pno" :time tim :dur 1/2 :pitch p)))

(fms:with-score
    (:filename *filename*
     :parts '((:id "pno" :inst "piano")))
  (fms:meas :time 0 :dur 3 :meas-divs '((3/2 3/2)))
  (notes))
