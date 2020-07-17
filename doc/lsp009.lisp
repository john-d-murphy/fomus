(defun notes ()
  (loop
     for o from 0 to 20 by 1/2
     do (fms:note :time o :dur 1/2
                  :pitch (if (< (random 1.0) 0.5) "wb1" "wb2"))))

(fms:with-score
    (:filename *filename*
     :percinsts '((:id "wb1" :template "low-woodblock" :perc-note 57)
                  (:id "wb2" :template "high-woodblock" :perc-note 64))
     :insts '((:id "pinst" :template "percussion" :percinsts ("wb1" "wb2")))
     :parts '((:id "perc" :name "Percussion" :inst "pinst")))
  (notes))
