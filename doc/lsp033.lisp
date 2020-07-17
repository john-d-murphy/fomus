(fms:with-score (:filename *filename*)
  (loop
     with h = (let ((x (list 0 3 4 6 8)))
                (loop repeat 10 nconc (sort (copy-list x)
                                	    (lambda (x y)
                                	      (declare (ignore x y))
                                	      (< (random 1.0) 0.5)))))
     for o from 0 below 12 by 1/2
     do
       (fms:note :time o :dur 1/2 :pitch (+ 67 (pop h)) :voice 1)
       (fms:note :time o :dur 1/2 :pitch (+ 55 (pop h)) :voice 2)))
