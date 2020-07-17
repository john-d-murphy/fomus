(fms:with-score
    (:filename *filename*
     :parts '((:id "apart" :inst "piano")))
  (loop repeat 3
     do (loop
           for off from 0 to 10 by 1/2
           do (fms:note :time off
                        :dur (if (< off 10) 1/2 1)
                        :pitch (+ 48 (random 25))
                        :voice '(1 2)))))
