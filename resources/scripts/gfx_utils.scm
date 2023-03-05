; GFX-Utils for sglthing
; This file is Public Domain, but credit would be appreciated!

(define (compile-shaders v f)
    (let (
        (vs (compile-shader v GL_VERTEX_SHADER))
        (fs (compile-shader f GL_FRAGMENT_SHADER))) 
        (link-program vs fs)))

(define (quick-load-model f)
    (load-model f)
    (get-model f))
    
(define (quick-load-texture f)
    (load-texture f)
    (get-texture f))

(io-add-directory "resources/gfxutils")
(io-add-directory "../resources/gfxutils")
    
(define normal-shader (compile-shaders "shaders/normal.vs" "shaders/fragment.fs"))
(define normal-simple-shader (compile-shaders "shaders/normal.vs" "shaders/fragment_simple.fs"))
(define rigged-shader (compile-shaders "shaders/rigged.vs" "shaders/fragment.fs"))
(define debug-shader  (compile-shaders "shaders/dbg.vs" "shaders/dbg.fs"))
(define sky-shader    (compile-shaders "shaders/sky.vs" "shaders/sky.fs"))
(define cloud-shader  (compile-shaders "shaders/cloud.vs" "shaders/cloud.fs"))
(define camera-light-area (lightarea-create))

(define sky-ball (quick-load-model "skyball.obj"))
(define sky-transform (make-transform))
(define cloud-transform (make-transform))
(set! (transform-sx cloud-transform) 50.0)
(set! (transform-sz cloud-transform) 50.0)
(update-transform cloud-transform)

(define (gfxutils-frame world camera)
    (set! (transform-px cloud-transform) (transform-px camera))
    (set! (transform-py cloud-transform) (+ 0.2 (transform-py camera)))
    (set! (transform-pz cloud-transform) (transform-pz camera))
    (update-transform cloud-transform)
    (set! (transform-px sky-transform) (transform-px camera))
    (set! (transform-py sky-transform) (transform-py camera))
    (set! (transform-pz sky-transform) (transform-pz camera))
    (update-transform sky-transform)
    (draw-sky world)
    (lightarea-update camera-light-area (transform-px camera) (transform-py camera) (transform-pz camera)))

(define (draw-sky world)
    (gl-no-depth) 
    (world-draw-object world sky-shader sky-ball sky-transform)
    (world-draw-primitive world cloud-shader 2 cloud-transform)
    (gl-yes-depth))

(engine-print "GFX-Utils loaded")