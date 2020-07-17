(defun notes ()
  (loop
     for tim from 0 below 10 by 1/2
     for p in '#1=(60 61 62 63 64 65 64 63 62 61 . #1#)
     do (fms:note :part "pno" :time tim :dur 1/2 :pitch p)))

(defun meass ()
  (fms:meas :time 0 :dur 5/2)
  (fms:meas :time 5 :dur 7/2))

(fms:with-score
    (:filename *filename*
     :sets '(:default-measdivs ((3 2) (4 3)))
     :parts '((:id "pno" :inst "piano")))
  (meass)
  (notes))
