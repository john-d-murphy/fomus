(defparameter *parts*
  '((:id "vla1" :inst "viola")
    (:id "vla2" :inst "viola")
    (:id "vla3" :inst "viola")))

(defparameter *metaparts*
  '((:id "vla" :parts ((:part "vla1" :from-voice 1 :to-voice 1)
                       (:part "vla2" :from-voice 2 :to-voice 1)
                       (:part "vla3" :from-voice 3 :to-voice 1)))))

(defun notes (reps dur pset)
  (loop
     with pat = (loop repeat reps
                   nconc (sort (copy-list pset)
                               (lambda (x y)
                                 (declare (ignore x y))
                                 (< (random 1.0) 0.5))))
     for c = (pop pat)
     for tim from 0 by dur
     while c
     do (loop
           for k in c
           do (fms:note :time tim :dur dur
                        :pitch k :part "vla"
                        :voice '(1 2 3)))))

(fms:with-score
    (:filename *filename*
     :parts *parts* :metaparts *metaparts*)
  (notes 10 1/4 '((60 62 64) (61 63 65) (62 64 66))))
