(defun notes ()
  (loop
     for o from 0 to 12 by 4
     do
       (fms:note :time o :dur 4 :pitch 72 :part "0")
       (fms:note :time o :dur 4 :pitch 48 :part "1")))

(defun marks ()
  (loop
     for basetime from 0 below 10 by (+ 4 1/2)
     do
       (fms:mark :time (+ basetime 1) :dur 2
                 :part "0" :marks '("<.." "..<"))
       (fms:mark :time (+ basetime 1 2) :dur 2
                 :part "0" :marks '(">.." "..>"))
       (fms:mark :time (+ basetime 3) :dur 2
                 :part "1" :marks '("<.." "..<"))
       (fms:mark :time (+ basetime 3 2) :dur 2
                 :part "1" :marks '(">.." "..>"))))

(fms:with-score
    (:filename *filename*
     :sets '(:wedges-canspanone t :wedges-cantouch t)
     :parts '((:inst "flute" :id "0")
              (:inst "tuba" :id "1")))
  (notes)
  (marks))
