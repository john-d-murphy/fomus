(fms:with-score
    (:filename *filename*
     :parts '((:id "prt1" :inst "piano" :vertmax 1))) ; overrides `vertmax' in piano instrument
  (loop
     with h = (let ((x (list 50 56 61 67)))
                (loop repeat 20 nconc (sort (copy-list x)
                                	    (lambda (x y)
                                	      (declare (ignore x y))
                                	      (< (random 1.0) 0.5)))))
     for o from 0 below 12 by 1/2
     do
       (fms:note :part 'prt1 :time o :dur 1/2 :pitch (pop h) :voice '(1 2))
       (fms:note :part 'prt1 :time o :dur 1/2 :pitch (pop h) :voice '(1 2))))
