(defun notes ()
  (loop
     for i from 0 below 24
     for tim from 0 by 1/3
     do (fms:note :time tim :dur 1/3
                  :pitch (+ 60 (random 13)))))

(defun meass ()
  (fms:meas :time 0 :dur 4 :min-tupletdur 2)
  (fms:meas :time 4 :dur 4 :max-tupletdur 1))

(fms:with-score (:filename *filename*)
  (notes)
  (meass))
