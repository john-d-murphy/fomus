(defun notes ()
  (loop
     for o from 0 to 16 by 1/2
     do
       (fms:note :time o :dur 1/2 :pitch 72 :part "0")
       (fms:note :time o :dur 1/2 :pitch 48 :part "1")))

(defun marks ()
  (loop
     for o from 0 to 20 by 5/2
     for mode in '("touch" "overlap" "include" "touch" "overlap" "include")
     for dur in '( 2       2         5/4       2       2         5/4)
     for part = t then (not part)
     do (fms:mark :time o :dur dur
                  :part (if part "0" "1")
                  :marks '("^")
                  :right mode)))

(fms:with-score
    (:filename *filename*
     :parts '((:inst "flute" :id "0")
              (:inst "tuba" :id "1")))
  (notes)
  (marks))
