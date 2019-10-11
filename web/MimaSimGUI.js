//<![CDATA[

TLN.append_line_numbers('source_code');

const MimaElementID = Object.freeze(
    {
        IR: 1,
        IAR: 2,
        TRA: 3,
        RUN: 4,
        SIR: 5,
        SAR: 6,
        ACC: 7,
        X: 8,
        Y: 9,
        Z: 10,
        ALU: 11,
        MICRO_CYCLE: 12,
        ONE: 13,
        IMMEDIATE: 14,
        MEMORY: 15,
        IOMEMORY: 16
    }
);

var MimaModel =
{
    IR: 0,
    IAR: 0,
    TRA: 0,
    RUN: 1,
    SIR: 0,
    SAR: 0,
    ACC: 0,
    X: 0,
    Y: 0,
    Z: 0,
    ALU: 0,
    MICRO_CYCLE: 1,
    ONE: 1,
};

var memoryLineMap = new Map();
var running = false;
var runTimerID;
var mima_instance;

Module.onRuntimeInitialized = async _ =>
{
    mima_instance = _mima_init();
    _mima_wasm_set_run(mima_instance, false);

    // cache could prevent this in firefox :(
    document.getElementById('btn_step').disabled = true;
    document.getElementById('btn_mstep').disabled = true;
    document.getElementById('btn_run').disabled = true;
    document.getElementById('btn_reset').disabled = true;

    document.getElementById('btn_compile').onclick = async function ()
    {
        memoryLineMap.clear();
        var value = document.getElementById('source_code').value;
        var ptr = allocate(intArrayFromString(value), 'i8', ALLOC_NORMAL);
        var res = _mima_compile_s(mima_instance, ptr);
        if (res)
        {
            document.getElementById('btn_step').disabled = false;
            document.getElementById('btn_mstep').disabled = false;
            document.getElementById('btn_run').disabled = false;
            document.getElementById('btn_reset').disabled = false;
            document.getElementById('btn_compile').disabled = true;
        }
        else
        {
            document.getElementById('btn_step').disabled = true;
            document.getElementById('btn_mstep').disabled = true;
            document.getElementById('btn_run').disabled = true;
            document.getElementById('btn_reset').disabled = true;
            document.getElementById('btn_compile').disabled = false;
        }
        _mima_wasm_free(ptr);
    };

    document.getElementById('btn_step').onclick = async function ()
    {
        _mima_run_instruction_step(mima_instance);
        _mima_wasm_set_run(mima_instance, false);
        resetMimaGUI();
    };

    document.getElementById('btn_mstep').onclick = async function ()
    {
        resetMimaGUI();
        _mima_wasm_set_run(mima_instance, true);
        _mima_run_micro_instruction_step(mima_instance);
        _mima_wasm_set_run(mima_instance, false);
    };

    document.getElementById('btn_run').onclick = async function ()
    {
        var id;
        if (running)
        {
            running = false;
            if (runTimerID)
                clearInterval(runTimerID);
            _mima_wasm_set_run(mima_instance, false);
            document.getElementById('btn_run').value = "Run";
            document.getElementById('btn_step').disabled = false;
            document.getElementById('btn_mstep').disabled = false;
            document.getElementById('btn_reset').disabled = false;
        }
        else
        {
            running = true;

            resetMimaGUI();
            _mima_wasm_set_run(mima_instance, true);
            runTimerID = setInterval(runLoop, 10);
            function runLoop()
            {
                resetMimaGUI();
                _mima_run_micro_instruction_step(mima_instance);
            }
            document.getElementById('btn_run').value = "Pause";
            document.getElementById('btn_step').disabled = true;
            document.getElementById('btn_mstep').disabled = true;
            document.getElementById('btn_reset').disabled = true;
        }
    };

    document.getElementById('btn_reset').onclick = function ()
    {
        document.getElementById('btn_reset').disabled = true;
        document.getElementById('btn_compile').disabled = false;
        document.getElementById('btn_step').disabled = true;
        document.getElementById('btn_mstep').disabled = true;
        document.getElementById('btn_run').disabled = true;

        var lastLine = document.getElementById('current_line');
        if (lastLine)
        {
            lastLine.setAttribute('style', 'background:#ffffff;');
            lastLine.removeAttribute('id');
        }

        resetMimaModel();
        resetMimaGUI();

        _mima_delete(mima_instance);
        
        mima_instance = _mima_init();
        _mima_wasm_set_run(mima_instance, false);

        _mima_wasm_set_log_level(2 /*INFO*/);
        document.getElementById('ll_info').checked = true;

        document.getElementById('log_text').innerHTML = "";
        document.getElementById('output_text').innerHTML = "";
        if (runTimerID)
            clearInterval(runTimerID);
    }

    document.getElementById('source_code').onchange = function ()
    {
        var line = document.getElementById('compile_error');
        if (line)
        {
            line.setAttribute("style", "background:#ffffff");
            line.removeAttribute("id");
        }
    }

    var ll_radio = document.forms.loglevel_radio_form;
    var prev = null;

    for (var i = 0; i < ll_radio.length; ++i)
    {
        ll_radio[i].addEventListener('change', function ()
        {
            if (this !== prev)
            {
                prev = this;
            }
            switch (this.value)
            {
            case "TRACE":
                _mima_wasm_set_log_level(0);
                break;
            case "INFO":
                _mima_wasm_set_log_level(2);
                break;
            case "WARN":
                _mima_wasm_set_log_level(3);
                break;
            }
        }
        );
    }

    _mima_wasm_set_log_level(2 /*INFO*/);
    document.getElementById('ll_info').checked = true;

    resetMimaGUI();
}

function resetMimaModel()
{
    MimaModel =
    {
        IR: 0,
        IAR: 0,
        TRA: 0,
        RUN: 1,
        SIR: 0,
        SAR: 0,
        ACC: 0,
        X: 0,
        Y: 0,
        Z: 0,
        ALU: -1,
        MICRO_CYCLE: 1,
        ONE: 1,
    };
    var svg = document.getElementById('mimaimg').contentDocument;
    svg.getElementById('ir_text').innerHTML = getHexString(MimaModel.IR);
    svg.getElementById('iar_text').innerHTML = getHexString(MimaModel.IAR);
    svg.getElementById('tra_label').setAttribute("fill", "#ffaaaa");
    svg.getElementById('run_label').setAttribute("fill", "#ffaaaa");
    svg.getElementById('sir_text').innerHTML = getHexString(MimaModel.SIR);
    svg.getElementById('sar_text').innerHTML = getHexString(MimaModel.SAR);
    svg.getElementById('acc_text').innerHTML = getHexString(MimaModel.ACC);
    svg.getElementById('x_text').innerHTML = getHexString(MimaModel.X);
    svg.getElementById('y_text').innerHTML = getHexString(MimaModel.Y);
    svg.getElementById('z_text').innerHTML = getHexString(MimaModel.Z);
    svg.getElementById('alu_text').innerHTML = getOpCodeName(MimaModel.ALU);
    svg.getElementById('counter_text').innerHTML = MimaModel.MICRO_CYCLE;
}

function getHexString(value)
{
    if (value < 0)
    {
        value = 0xFFFFFFFF + value + 1;
    }
    return "0x" + ("00000000" + value.toString(16).toUpperCase()).substr(-8);
}

function getOpCodeName(opCode)
{
    switch (opCode)
    {
    case -1:
        return "ALU";
    case 0:
        return "ADD";
    case 1:
        return "AND";
    case 2:
        return "OR";
    case 3:
        return "XOR";
    case 4:
        return "LDV";
    case 5:
        return "STV";
    case 6:
        return "LDC";
    case 7:
        return "JMP";
    case 8:
        return "JMN";
    case 9:
        return "EQL";
    case 0xF0:
        return "HLT";
    case 0xF1:
        return "NOT";
    case 0xF2:
        return "RAR";
    case 0xF3:
        return "RRN";
    }
}

function setHalted()
{
    document.getElementById('btn_run').value = "Run";
    document.getElementById('btn_run').disabled = true;
    document.getElementById('btn_step').disabled = true;
    document.getElementById('btn_mstep').disabled = true;
    document.getElementById('btn_reset').disabled = false;
    running = false;
    if (runTimerID)
        clearInterval(runTimerID);
}

function hitBreakpoint()
{
    document.getElementById('btn_run').value = "Run";
    document.getElementById('btn_run').disabled = false;
    document.getElementById('btn_step').disabled = false;
    document.getElementById('btn_mstep').disabled = false;
    document.getElementById('btn_reset').disabled = false;
    document.getElementById('current_line').setAttribute('style', 'background:#eeaaff;');
    running = false;
    if (runTimerID)
        clearInterval(runTimerID);
}

function resetMimaGUI()
{
    var svg = document.getElementById('mimaimg').contentDocument;
    
    svg.getElementById('darrow_ir_bus').setAttribute('visibility', 'visible');
    svg.getElementById('darrow_iar_bus').setAttribute('visibility', 'visible');
    svg.getElementById('darrow_sir_bus').setAttribute('visibility', 'visible');

    svg.getElementById('darrow_sar_bus').setAttribute('visibility', 'visible');
    svg.getElementById('darrow_sir_mem').setAttribute('visibility', 'visible');
    svg.getElementById('darrow_sir_iobus').setAttribute('visibility', 'visible');

    svg.getElementById('darrow_acc_bus').setAttribute('visibility', 'visible');

    svg.getElementById('arrow_bus_acc').setAttribute('visibility', 'hidden');
    svg.getElementById('arrow_acc_bus').setAttribute('visibility', 'hidden');

    svg.getElementById('arrow_ir_bus').setAttribute('visibility', 'hidden');
    svg.getElementById('arrow_bus_ir').setAttribute('visibility', 'hidden');
    
    svg.getElementById('arrow_iar_bus').setAttribute('visibility', 'hidden');
    svg.getElementById('arrow_bus_iar').setAttribute('visibility', 'hidden');
    
    svg.getElementById('arrow_sir_bus').setAttribute('visibility', 'hidden');
    svg.getElementById('arrow_bus_sir').setAttribute('visibility', 'hidden');
    
    svg.getElementById('arrow_sar_bus').setAttribute('visibility', 'hidden');
    svg.getElementById('arrow_bus_sar').setAttribute('visibility', 'hidden');
    
    svg.getElementById('arrow_sir_mem').setAttribute('visibility', 'hidden');
    svg.getElementById('arrow_mem_sir').setAttribute('visibility', 'hidden');
    
    svg.getElementById('arrow_sir_iobus').setAttribute('visibility', 'hidden');
    svg.getElementById('arrow_iobus_sir').setAttribute('visibility', 'hidden');
    
    svg.getElementById('bus').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-dasharray:none');
    svg.getElementById('iobus').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-dasharray:none');
    svg.getElementById('mem_color').setAttribute('style', 'fill:#e1d5e7;stroke:#9673a6;stroke-width:2;stroke-miterlimit:10');
    svg.getElementById('alu_shape').setAttribute('style', 'fill:#bfbfbf;stroke:#4c4c4c;stroke-width:0;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none');

    svg.getElementById('arrow_bus_x').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-miterlimit:10');
    svg.getElementById('arrow_bus_y').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-miterlimit:10');

    svg.getElementById('arrow_x_alu').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-miterlimit:10');
    svg.getElementById('arrow_y_alu').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-miterlimit:10');
    svg.getElementById('arrow_alu_z').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-miterlimit:10');

    svg.getElementById('arrow_z_bus').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-miterlimit:10');
    svg.getElementById('arrow_one_bus').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-miterlimit:10');

    svg.getElementById('arrow_sar_iobus').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-miterlimit:10');
    svg.getElementById('arrow_sar_mem').setAttribute('style', 'fill:#ffffff;stroke:#000000;stroke-miterlimit:10');

    svg.getElementById('alu_text').innerHTML = "ALU";

    svg.getElementById('counter_text').innerHTML = "1";
}

function updateMimaState(source, target, value)
{
    var svg = document.getElementById('mimaimg').contentDocument;
    switch (target)
    {
    case 1:
        MimaModel.IR = value;
        svg.getElementById('ir_text').innerHTML = getHexString(value);
        svg.getElementById('darrow_ir_bus').setAttribute('visibility', 'hidden');
        svg.getElementById('arrow_bus_ir').setAttribute('visibility', 'visible');

        svg.getElementById('darrow_sir_bus').setAttribute('visibility', 'hidden');
        svg.getElementById('arrow_sir_bus').setAttribute('visibility', 'visible');
        svg.getElementById('bus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-dasharray:none');
        break;
    case 2:
        MimaModel.IAR = value;
        svg.getElementById('iar_text').innerHTML = getHexString(value);
        let line = memoryLineMap.get(value);
        if (!line)
            break;
        var lastLine = document.getElementById('current_line');
        if (lastLine)
        {
            lastLine.setAttribute('style', 'background:#ffffff;');
            lastLine.removeAttribute('id');
        }
        document.querySelector('.tln-wrapper :nth-child(' + line + ')').setAttribute('style', 'background:#eeeeff;');
        document.querySelector('.tln-wrapper :nth-child(' + line + ')').setAttribute('id', 'current_line');
        if (source == MimaElementID.IR)
        {
            svg.getElementById('darrow_ir_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_ir_bus').setAttribute('visibility', 'visible');
        }
        if (source == MimaElementID.Z)
        {
            svg.getElementById('arrow_z_bus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-miterlimit:10');
        }
        svg.getElementById('darrow_iar_bus').setAttribute('visibility', 'hidden');
        svg.getElementById('arrow_bus_iar').setAttribute('visibility', 'visible');
        svg.getElementById('bus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-dasharray:none');
        break;
    case 3:
        MimaModel.TRA = value;
        svg.getElementById('tra_label').setAttribute("class", value ? "active" : "inactive");
        break;
    case 4:
        MimaModel.RUN = value;
        svg.getElementById('run_label').setAttribute("class", value ? "active" : "inactive");
        break;
    case 5:
        MimaModel.SIR = value;
        svg.getElementById('sir_text').innerHTML = getHexString(value);
        
        if (source == MimaElementID.MEMORY)
        {
            svg.getElementById('darrow_sir_mem').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_mem_sir').setAttribute('visibility', 'visible');
            svg.getElementById('mem_color').setAttribute('style', 'fill:#aaffaa;stroke:#9673a6;stroke-width:2;stroke-miterlimit:10');
            svg.getElementById('mem_text').innerHTML = "Read done";
        }
        
        if (source == MimaElementID.IOMEMORY)
        {
            svg.getElementById('darrow_sir_iobus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_iobus_sir').setAttribute('visibility', 'visible');
            svg.getElementById('iobus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-dasharray:none');
        }
        
        if (source == MimaElementID.ACC)
        {
            svg.getElementById('darrow_sir_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_bus_sir').setAttribute('visibility', 'visible');
            svg.getElementById('darrow_acc_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_acc_bus').setAttribute('visibility', 'visible');
            svg.getElementById('bus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-dasharray:none');
        }
        break;
    case 6:
        MimaModel.SAR = value;
        svg.getElementById('sar_text').innerHTML = getHexString(value);
        svg.getElementById('darrow_sar_bus').setAttribute('visibility', 'hidden');
        svg.getElementById('arrow_bus_sar').setAttribute('visibility', 'visible');
        svg.getElementById('bus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-dasharray:none');
        
        if (source == MimaElementID.IR)
        {
            svg.getElementById('darrow_ir_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_ir_bus').setAttribute('visibility', 'visible');
        }
        
        if (source == MimaElementID.IAR)
        {
            svg.getElementById('darrow_iar_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_iar_bus').setAttribute('visibility', 'visible');
        }

        // IO or normal memory?
        if (value > 0xC000000)
        {
            svg.getElementById('arrow_sar_iobus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-miterlimit:10');
        }
        else
        {
            svg.getElementById('arrow_sar_mem').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-miterlimit:10');
            svg.getElementById('mem_color').setAttribute('style', 'fill:#aaffaa;stroke:#9673a6;stroke-width:2;stroke-miterlimit:10');
            svg.getElementById('mem_text').innerHTML = "waiting...";
        }
        break;
    case 7:
        MimaModel.ACC = value;
        svg.getElementById('acc_text').innerHTML = getHexString(value);
        svg.getElementById('darrow_acc_bus').setAttribute('visibility', 'hidden');
        svg.getElementById('arrow_bus_acc').setAttribute('visibility', 'visible');
        svg.getElementById('bus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-dasharray:none');
        if (source == MimaElementID.IR)
        {
            svg.getElementById('darrow_ir_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_ir_bus').setAttribute('visibility', 'visible');
        }
        if (source == MimaElementID.SIR)
        {
            svg.getElementById('darrow_sir_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_sir_bus').setAttribute('visibility', 'visible');
        }
        if (source == MimaElementID.Z)
        {
            svg.getElementById('arrow_z_bus').setAttribute('visibility', 'visible');
        }
        break;
    case 8:
        MimaModel.X = value;
        svg.getElementById('x_text').innerHTML = getHexString(value);
        svg.getElementById('arrow_bus_x').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-miterlimit:10');
        svg.getElementById('bus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-dasharray:none');
        if (source == MimaElementID.ACC)
        {
            svg.getElementById('darrow_acc_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_bus_acc').setAttribute('visibility', 'visible');
        }
        if (source == MimaElementID.IAR)
        {
            svg.getElementById('darrow_iar_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_iar_bus').setAttribute('visibility', 'visible');
        }
        break;
    case 9:
        MimaModel.Y = value;
        svg.getElementById('y_text').innerHTML = getHexString(value);
        svg.getElementById('arrow_bus_y').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-miterlimit:10');
        svg.getElementById('bus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-dasharray:none');
        if (source == MimaElementID.IR)
        {
            svg.getElementById('darrow_ir_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_ir_bus').setAttribute('visibility', 'visible');
        }
        if (source == MimaElementID.SIR)
        {
            svg.getElementById('darrow_sir_bus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_sir_bus').setAttribute('visibility', 'visible');
        }
        if (source == MimaElementID.ONE)
        {
            svg.getElementById('arrow_one_bus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-miterlimit:10');
        }
        break;
    case 10:
        MimaModel.Z = value;
        svg.getElementById('z_text').innerHTML = getHexString(value);

        svg.getElementById('arrow_alu_z').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-miterlimit:10');
        svg.getElementById('arrow_x_alu').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-miterlimit:10');
        svg.getElementById('arrow_y_alu').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-miterlimit:10');
        svg.getElementById('alu_shape').setAttribute('style', 'fill:#aaffaa;stroke:#4c4c4c;stroke-width:0;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none');

        break;
    case 11:
        MimaModel.ALU = value;
        svg.getElementById('alu_text').innerHTML = getOpCodeName(value);
        
        svg.getElementById('alu_shape').setAttribute('style', 'fill:#aaffaa;stroke:#4c4c4c;stroke-width:0;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none');
        break;
    case 12:
        MimaModel.MICRO_CYCLE = value;
        svg.getElementById('counter_text').innerHTML = value;
        break;
    case 15: //MEMORY
        svg.getElementById('darrow_sir_mem').setAttribute('visibility', 'hidden');
        svg.getElementById('arrow_sir_mem').setAttribute('visibility', 'visible');
        svg.getElementById('mem_color').setAttribute('style', 'fill:#aaffaa;stroke:#9673a6;stroke-width:2;stroke-miterlimit:10');
        break;
    case 16: //IOMEMORY
        svg.getElementById('iobus').setAttribute('style', 'fill:#aaffaa;stroke:#000000;stroke-dasharray:none');

        if (source == MimaElementID.SIR)
        {
            svg.getElementById('darrow_sir_iobus').setAttribute('visibility', 'hidden');
            svg.getElementById('arrow_sir_iobus').setAttribute('visibility', 'visible');
        }
        break;
    }
};
//]]>
