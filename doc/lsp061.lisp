(defun notes (n r)
  (loop
     repeat n
     for tim from 0 by r
     for stf = 1 then (- 3 stf)
     and clf in '#1=("treble" "treble" "treble" "treble" "bass" "bass" "bass" "bass" . #1#)
     do (fms:note :time tim :dur r :pitch 60 :staff stf :clef clf)))

(fms:with-score (:filename *filename*)
  (notes 16 1/2))
