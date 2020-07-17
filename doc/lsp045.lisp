(fms:with-score
    (:filename *filename*
     :sets '(:transpose-keysigs nil
             :wedges-canspanone t
             :wedges-cantouch t)
     :parts '((:id tpt :inst :bflat-trumpet)))
  (loop
     with cresc = t
     for off from 0 to 10 by 2
     do (fms:note :time off
                  :dur 2
                  :pitch (+ 70 (random 5))
                  :marks (when (< (random 1.0) 0.5)
                           (prog1
                               (if cresc '("<..") '(">.."))
                             (setf cresc (not cresc)))))))
