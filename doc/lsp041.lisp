(fms:with-score
    (:filename *filename*
     :parts '((:id pno :inst :piano)))
  (loop
     with sluron
     for off from 0 to 10 by 1/2
     do (fms:note :time off
                  :dur (if (< off 10) 1/2 1)
                  :pitch (+ 60 (random 25))
                  :marks (when (< (random 1.0) 0.5)
                           (prog1
                               (if sluron '("..)") '("(.."))
                             (setf sluron (not sluron)))))))
