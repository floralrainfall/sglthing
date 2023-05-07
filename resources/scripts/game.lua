local engine_time = 0.0
local test_shader = asset.program(
    asset.shader("shaders/normal.vs", gl.vertex_shader),
    asset.shader("shaders/fragment.fs", gl.fragment_shader)
)
local test_matrix = matrix.new()
local test_model = asset.model("yaal/models/bomb.obj")

matrix.identity(test_matrix)

function ScriptFrame(game)
    engine_time = world.time(game)
end

function ScriptFrameRender(game, light_pass)
    gl.render_model(game, test_model, test_matrix, test_shader)
end

function ScriptFrameUI(game, ui_ref)
    
end