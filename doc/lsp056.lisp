(defun notes ()
  (loop
     for o from 0 to 20 by 1
     do (fms:note :time o :dur 1
                  :pitch (if (< o 12) "bdr" "cym"))))

(fms:with-score
    (:filename *filename*
     :percinsts '((:id "bdr" :perc-note 60 :perc-name "B. Drum")
                  (:id "cym" :perc-note 60 :perc-name "Cymbal"))
     :insts '((:id "pinst" :template "percussion" :percinsts ("bdr" "cym")))
     :parts '((:id "perc" :name "Percussion" :inst "pinst")))
  (notes))
