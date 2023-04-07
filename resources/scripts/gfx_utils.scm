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
    (define camera-px (transform-px camera))
    (define camera-py (transform-py camera))
    (define camera-pz (transform-pz camera))
    (set! (transform-px cloud-transform) camera-px)
    (set! (transform-py cloud-transform) (+ 0.2 camera-py))
    (set! (transform-pz cloud-transform) camera-pz)
    (update-transform cloud-transform)
    (set! (transform-px sky-transform) camera-px)
    (set! (transform-py sky-transform) camera-py)
    (set! (transform-pz sky-transform) camera-pz)
    (update-transform sky-transform)
    (draw-sky world)
    (lightarea-update camera-light-area camera-px camera-py camera-pz))

(define (gfxutils-set-animator pass animator shader world)
    (when pass (animator-set-uniforms animator (world-lighting-shader world)))
    (when (not pass) (animator-set-uniforms animator shader)))

(define (draw-sky world)
    (gl-no-depth) 
    (world-draw-object world sky-shader sky-ball sky-transform)
    (world-draw-primitive world cloud-shader 2 cloud-transform)
    (gl-yes-depth))

(engine-print "GFX-Utils loaded")