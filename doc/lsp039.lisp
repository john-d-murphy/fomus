(fms:with-score
    (:filename *filename*
     :parts '((:id ce :inst cello)))
  (loop
     with marks = '(()
                    (("x" "text")) () () ()
                    (("x^" "text above")) () () ()
                    (("x_" "text below")) () () ()
                    (("x!" "staff text")) () () ()
                    () () () ())
     repeat 21
     for m in marks
     for tim from 0 by 1/2
     do (fms:note :time tim
                  :dur (if (< tim 10) 1/2 1)
                  :pitch (+ 36 (random 3))
                  :marks m)))
