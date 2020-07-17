(defun notes ()
  (loop
     for o from 0 to 20 by 1/2
     do
       (fms:note :time o :dur 1/2 :pitch 72 :part "0")
       (fms:note :time o :dur 1/2 :pitch 48 :part "1")))

(defun marks ()
  (loop
     repeat 8
     do (fms:mark :time (/ (random 40) 2) :dur 0
                  :part (if (< (random 1.0) 0.5) "0" "1")
                  :marks '("^"))))

(fms:with-score
    (:filename *filename*
     :parts '((:inst "flute" :id "0")
              (:inst "tuba" :id "1")))
  (notes)
  (marks))
