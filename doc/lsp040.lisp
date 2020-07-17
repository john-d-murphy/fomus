(fms:with-score
    (:filename *filename*
     :sets '(:mark-texts ("vib" "vibrato" "moltovib" "molto vibrato"))
     :parts '((:id ce :inst cello)))
  (loop
     with marks = '(("vib") ("moltovib") nil nil)
     repeat 21
     for tim from 0 by 1/2
     do (fms:note :time tim
                  :dur (if (< tim 10) 1/2 1)
                  :pitch (+ 48 (random 3))
                  :marks (nth (floor tim 3) marks))))
