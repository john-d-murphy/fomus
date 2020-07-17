(defun notes (len minp maxp)
  (loop
     repeat len
     for tim from 0 by 1/4
     do (fms:note :time tim
                  :dur 1/4
                  :pitch (+ minp (random (- maxp minp)))
                  :voice '(1 2))))

(fms:with-score
    (:filename *filename*
     :parts '((:id "apart" :inst "piano")))
  (notes 40 50 80)
  (notes 40 40 70))
