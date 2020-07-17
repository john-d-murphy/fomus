(defun notes ()
  (loop
     for o from 0 to 10 by 1/2
     do
       (fms:note :time o :dur 1/2 :pitch 72 :part "0")
       (fms:note :time o :dur 1/2 :pitch 48 :part "1")))

(defun marks ()
  (loop
     for basetime from 0 below 10 by 5
     do
       (fms:mark :time (+ basetime 1) :dur 3
                 :part "0" :marks '("(.." "..)"))
       (fms:mark :time (+ basetime 3) :dur 3
                 :part "1" :marks '("(.." "..)"))))

(fms:with-score
    (:filename *filename*
     :parts '((:inst "flute" :id "0")
              (:inst "tuba" :id "1")))
  (notes)
  (marks))
