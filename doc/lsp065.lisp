(defun notes ()
  (fms:note :time 0 :dur 2 :pitch 70))

(fms:with-score
    (:filename *filename*
     :parts '((:id "mypart"
               :inst (:template "viola"
        	      :name "Electric Viola" :abbr "evla"
        	      :min-pitch 48 :max-pitch 108
        	      :open-strings (48 55 62 69)))))
  (notes))
