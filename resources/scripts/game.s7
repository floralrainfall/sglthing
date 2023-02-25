(load-model "box.obj")
(define box-model (get-model "box.obj"))

(define (compile-shaders v f)
    (let (
        (vs (compile-shader v GL_VERTEX_SHADER))
        (fs (compile-shader f GL_FRAGMENT_SHADER))) 
        (link-program vs fs)))

(define box-shader (compile-shaders "shaders/normal.vs" "shaders/fragment.fs"))

(define (script-frame world)
    ())
(define (script-frame-render world)
    (world-draw-object world box-shader box-model))