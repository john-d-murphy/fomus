(defun notes ()
  (fms:note :time 0 :dur 2 :pitch 70))

(fms:with-score
    (:filename *filename*
     :parts '((:id "mypart"
               :inst (:name "Electric Viola" :abbr "evla"
        	      :min-pitch 48 :max-pitch 108
        	      :open-strings (48 55 62 69)
        	      :staves ((:clefs ((:instclef alto :octs-up 0)
        				(:instclef treble :octs-down 0))))))))
  (notes))
