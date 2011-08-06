(ns core
  (:use [lamina core]
        [gloss core]
        [aleph tcp udp]))

#_ (def port 9000)
(println "PORT: " port)
(def port (inc port))

(def sv (udp-socket {:frame (string :utf-8 :delimiters ["\n"] :as-str true) :port port }))
(def client (udp-socket {:frame (string :utf-8 :delimiters ["\n"] :as-str true)}))

(def out *out*)
(defn udp-handler [s m]
  (.write out (str m "\n"))
  (.flush out)
  )

#_ (receive-all @sv
                (fn [m] 
                  #_ (require 'core :reload)
                  (udp-handler @sv m)))
#_ (enqueue @client {:port port :host "127.0.0.1" :message "GET 1 1"})
#_ (enqueue @client {:port port :host "192.168.1.110" :message "GET 1 1"})
#_ (enqueue @client {:port port :host "172.16.133.1" :message "GET 1 1"})

(defn send-msg [msg]
  (enqueue @client {:port port :host "172.16.133.159" :message msg}))

