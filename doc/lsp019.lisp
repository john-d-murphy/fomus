(defun notes (n)
  (loop
     repeat n
     for tim from 0
     do (fms:note :time tim :dur 1
                  :pitch (+ 48 (random 25)))))

(defun gracenotes (n)
  (loop
     repeat n
     for tim from 0
     do (loop
           repeat (random 3)
           for g from 0 by 1/4
           do (fms:note :time tim :dur 1/4
                        :grace g
                        :pitch (+ 48 (random 25))))))

(fms:with-score (:filename *filename*)
  (notes 20)
  (gracenotes 16))
