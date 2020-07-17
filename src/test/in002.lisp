(fms:with-score ()
  (loop
     repeat 2
     do (loop
	   for o from 0 below 20 by 1/2
	   do (fms:note :time o :dur 1/2 :pitch (+ 40 (random 40)) :voice '(1 2)))))
