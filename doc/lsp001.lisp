(fms:with-score
    (:filename *filename*
     :sets '(:cautaccs nil) ; score-level settings keyword/value pairs
     :run t) ; unnecessary, this is true by default
  (loop
     for o from 0 below 20 by 1/2
     do (fms:note :time o :dur 1/2 :pitch (+ 65 (random 15))))) ; note entry function
