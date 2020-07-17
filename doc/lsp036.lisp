(fms:with-score (:filename *filename*)
  (fms:meas :time 0 :dur 2 :comp t :keysig "gmaj")
  (loop
     for n from 0 below 20
     for tim from 0 by 1/3
     do (fms:note :time tim
                  :dur 1/3
                  :pitch (+ 55 (* 11 (sin tim)))
                  :staff (if (< n 10) 2 1)
                  :acc (if (< n 10) '(n f) '(n s)))))
