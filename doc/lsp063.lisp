(defun notes ()
  (loop
     for tim from 0 by 1/5
     and tupcnt from 0
     for tupmod = (mod tupcnt 10)
     while (< tim 8)
     do (fms:note :time tim :dur 1/5
                  :pitch (+ 60 (random 13))
        	  :marks (append (when (= tupmod 0) '("tup.."))
        			 (when (= tupmod 9) '("..tup")))
        	  :sets '(:tupletdur 2 :tupletrat 5/4))))

(fms:with-score (:filename *filename*)
  (notes))
