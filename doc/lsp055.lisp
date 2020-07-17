(defun notes ()
  (loop
     for o from 0 to 20 by 1/2
     do (fms:note :time o :dur 1/2
                  :pitch (if (< (random 1.0) 0.5) "wb1" "wb2"))))

(fms:with-score
    (:filename *filename*
     :parts '((:id "perc" :name "Percussion"
               :inst (:template "percussion" :percinsts
                      ((:id "wb1"
                        :template "low-woodblock"
                        :perc-note 57
                        :perc-voice 2)
                       (:id "wb2"
                        :template "high-woodblock"
                        :perc-note 64
                        :perc-voice 1))))))
  (notes))
