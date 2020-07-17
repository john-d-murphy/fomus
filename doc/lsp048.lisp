(defun notes ()
  (loop
     for o from 0 to 20 by 1/2
     do
       (fms:note :time o :dur 1/2 :pitch 72 :voice 1)
       (fms:note :time o :dur 1/2 :pitch 48 :voice 2)))

(defun marks ()
  (loop
     repeat 8
     do (fms:mark :time (/ (random 40) 2) :dur 1/2
                  :voice (if (< (random 1.0) 0.5) 1 2)
                  :marks '("^"))))

(fms:with-score
    (:filename *filename*
     :parts '((:id pno :inst "piano")))
  (notes)
  (marks))
