(fms:with-score
    (:filename *filename*
     :parts '((:id "apart" :inst "piano")))
  (loop
     repeat 10
     for tim = (random 20.0)
     and dur = (+ 1 (random 3.0))
     do (fms:note :time tim
                  :dur dur
                  :pitch (+ 60 (random 25))
                  :voice '(1 2 3))))
