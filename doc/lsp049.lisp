(fms:with-score
    (:filename *filename*
     :parts '((:id pno :inst :piano)))
  (loop
     for off from 0 to 10 by 1/2
     for slurend = (< (random 1.0) 0.333)
     for sluron = (if slurend (< (random 1.0) 0.5) sluron)
     do (fms:note :time off
                  :dur (if (< off 10) 1/2 1)
                  :pitch (+ 60 (random 25))
                  :marks (append
                          (when (and slurend sluron) '(("(..")))
                          (when sluron '((".(.")))))))
