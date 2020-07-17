(fms:with-score
    (:filename *filename*
     :parts '((:id pno :inst :piano)))
  (loop
     for v from 1 to 2
     do (loop
           for off from 0 to 10 by 1/2
           do (fms:note :time off
                        :dur (if (< off 10) 1/2 1)
                        :pitch (ecase v
                                 (1 (+ 60 (random 25)))
                                 (2 (+ 35 (random 25))))
                        :voice v
                        :marks (if (< (random 1.0) 0.5)
                                   '("(..")
                                   '())))))
