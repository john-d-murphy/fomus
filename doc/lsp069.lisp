(defun notes (n)
  (loop
     repeat n
     for tim from 0 by 1/2
     for p = 60 then (+ p (random 7) -3)
     do (fms:note :part "pno" :time tim :dur 1/2 :pitch p)))

(defun meass ()
  (loop
     with tim = 0
     for dur in (list 2 (+ 3 1/2) (+ 4 1/4))
     do
       (fms:meas :time tim :dur dur)
       (incf tim dur)))

(fms:with-score
    (:filename *filename*
     :sets '(:timesig-den 8)
     :parts '((:id "pno" :inst "piano")))
  (meass)
  (notes 18))
