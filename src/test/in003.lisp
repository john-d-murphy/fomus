(fms:with-score
    (
     :filename "/tmp/tmp.ly"
     :parts '((:id mypart :inst OBOE :max-pitch 100))
     :sets '(:double-accs nil)
     :run t
     )
  (fms:meas :time 0 :dur 3)
  (loop
     for o from 0 below 20 by 1/2
     do (fms:note  :part 'mypart :time o :dur 1/2 :pitch (+ 65 (random 15)))))
