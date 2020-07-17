(defun notes ()
  (loop
     for tim from 0 below 16 by 1/2
     for p in '#1=(60 62 64 65 64 62 60 59 . #1#)
     do (fms:note :time tim :dur 1/2 :pitch p)))

(defun meass ()
  (loop
     for tim from 0 to 12 by 3
     when (/= tim 9)
     do (fms:meas :time tim :dur 4)))

(fms:with-score
    (:filename *filename*
     :sets '(:min-measdur 3))
  (meass)
  (notes))
