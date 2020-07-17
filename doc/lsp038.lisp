(defun blast-notes (list base upto)
  (loop
     with time = 0
     do
       (loop
          for pitch in list
          do
            (fms:note :part 'pt1 :time time :pitch (+ base pitch) :dur 1/2)
            (incf time 1/2)
          when (>= time upto) do (return-from blast-notes))
       (incf base)))

(fms:with-score (:name 'main ; the main score
                 :filename *filename*)
  (fms:with-scores ((:name 'chunk1
                     :parts '((:id pt1 :inst guitar)) ; a score chunk
                     :run nil)
                    (:name 'chunk2
                     :parts '((:id pt1 :inst guitar)) ; another score chunk
                     :run nil))
    (fms:with-score chunk1 ; call same function on different chunks
      (blast-notes '(0 2 1 4) 55 (+ 7 1/2)))
    (fms:with-score chunk2
      (blast-notes '(0 3 4 2) 56 (+ 7 1/2)))
    (fms:merge :to 'main :from 'chunk1 :time 1) ; insert the chunks at time offsets 1 and 9
    (fms:merge :to 'main :from 'chunk2 :time 9)))
