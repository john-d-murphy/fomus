(fms:with-score
    (:filename *filename*
     :parts '((:id "apart" :inst "piano")))
  (loop
     with marks = '(() (".") (">") ("^") ("!") ("/."))
     repeat 21
     for tim from 0 by 1/2
     do (fms:note :time tim
                  :dur (if (< tim 10) 1/2 1)
                  :pitch (+ 48 (random 25))
                  :marks (nth (random (length marks)) marks))))
