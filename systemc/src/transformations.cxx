#include <array>
#include <filesystem>
#include <utils.hxx>
#include <common.hxx>
#include <transformations.hxx>

namespace streebog_hw 
{

constexpr int u64_count = 512 / 64;

void sc_uint512_to_uint64_ts(uint64_t *array, int64_t size, u512 from)
{
    for (int i = 0; i < u64_count; i++)
    {
        array[i] = from.to_uint64() & 0xFFFFFFFFFFFFFFFF;
        from >>= 64;
    }
}

u512 uint64_ts_to_sc_uint512(uint64_t *array, int64_t size)
{
    u512 result = 0;
    for (int i = u64_count - 1; i >= 0; i--)
    {
        result <<= 64;
        result  |= array[i] & 0xFFFFFFFFFFFFFFFF;
    }

    return result;
}

PTransform::PTransform(sc_core::sc_module_name const &name)
    : AUTONAME(a_i)
    , AUTONAME(result_o)
{
    result_o.bind(result_s_);
    SC_METHOD(method);
    sensitive << a_i;
}

void PTransform::method()
{
    u512 result = 0;
    std::array<unsigned char, BLOCK_SIZE> a_bytes;
    std::array<unsigned char, BLOCK_SIZE> result_bytes;
    
    sc_uint512_to_bytes(a_bytes.data(), a_bytes.size(), a_i->read());

    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        result_bytes[i] = a_bytes[TAU[i]];
    }
    
    result_s_.write(bytes_to_sc_uint512(result_bytes.data(), result_bytes.size()));
}

SLTransform::SLTransform(sc_core::sc_module_name const &name)
    : AUTONAME(a_i)
    , AUTONAME(result_o)
{
    result_o.bind(result_s_);
    SC_METHOD(method);
    sensitive << a_i;
}

void SLTransform::method()
{
    std::array<uint64_t, BLOCK_SIZE> a_qw;
    std::array<uint64_t, BLOCK_SIZE> result_qw;
    
    sc_uint512_to_uint64_ts(a_qw.data(), a_qw.size(), a_i->read());

    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        uint64_t c = 0;
        for (int64_t j = 0; j < sizeof(c); j++)
        {
            uint8_t byte = (a_qw[i] >> (j * 8)) & 0xFF;
            c ^= sl_precomp_table[j][byte];
        }

        result_qw[i] = c;
    }

    result_s_.write(uint64_ts_to_sc_uint512(result_qw.data(), result_qw.size()));
}

Gn::Gn(sc_core::sc_module_name const &name)
    : AUTONAME(m_i)
    , AUTONAME(n_i)
    , AUTONAME(h_i)
    , AUTONAME(start_i)
    , AUTONAME(ack_i)
    , AUTONAME(result_o)
    , AUTONAME(state_o)
    , AUTONAME(sl_tr_a_o)
    , AUTONAME(sl_tr_result_i)
    , AUTONAME(p_tr_a_o)
    , AUTONAME(p_tr_result_i)
{
    result_o.bind(result_s_);
    state_o.bind(state_s_);
    sl_tr_a_o.bind(sl_tr_a_s_);
    p_tr_a_o.bind(p_tr_a_s_);

    SC_THREAD(thread);

    state_s_.write(State::CLEAR);
}

void Gn::thread()
{
    while (true)
    {
        switch (state_s_.read())
        {
            case State::CLEAR:
                {
                    result_s_.write(0);
                    sl_tr_a_s_.write(0);
                    p_tr_a_s_.write(0);

                    WAIT_WHILE(start_i->read() == 0);

                    advance_state(State::BUSY);
                    break;
                }
            case State::BUSY:
                {
                    result_s_.write(compute_gn());
                    advance_state(State::DONE);
                    break;
                }
            case State::DONE:
                {
                    WAIT_WHILE(ack_i->read() == 0);

                    advance_state(State::CLEAR);
                    break;
                }
        }
    }
}

void Gn::advance_state(State next_state)
{
    state_s_.write(next_state);
    WAIT_WHILE(state_s_ != next_state);
}

// What you are about to see here is insane.
// keep in mind, that this function was intentionally 
// written in this way, to mimic a multicycle process 
// of computing the value of G_n. I do understand, that reading
// this is a challenge. I do admit, that this is not a
// good function. But I made my best to make it at least somewhat
// readable.
// The reason for this is simple: this function will later
// be implemented in systemverilog, so I wanted it to resemble 
// systemverilog style.
u512 Gn::compute_gn()
{
    enum C_STEPS
    {
        K_I,
        E_STEP1,
        E_STEP2,
        E_STEP3,
        E_STEP4,
        G_N_STEP1,
        G_N_STEP2,
    } cstep = G_N_STEP1;

    u512 r1 = 0;

    u512 E_STEP1_k = 0;
    u512 E_STEP1_m = 0;

    u512 E_STEP2_prev_k = 0;

    u512 E_STEP3_prev_k = 0;
    u512 E_STEP3_new_m = 0;

    u512 E_STEP4_prev_k = 0;

    u8 K_I_i = 0;
    u512 K_I_prev_k = 0;

    u512 G_N_STEP2_E = 0; 

    u512 result = 0;

    bool finished = false;
    while (!finished)
    {
        switch (cstep)
        {
            case K_I:
                {
                    const u512 &prev_k = K_I_prev_k;
                    const u8 &i = K_I_i;

                    DEBUG_OUT << "K_I " << "i = " << u8{i + 1}.to_string(sc_dt::SC_DEC) << std::endl;
                    DEBUG_OUT << "K_I " << "prev_k = " << prev_k.to_string(sc_dt::SC_HEX) << std::endl;

                    r1 = prev_k ^ *C[i.to_int() - 1];

                    p_tr_a_s_.write(r1);
                    wait_clk(1);

                    sl_tr_a_s_.write(p_tr_result_i->read());
                    wait_clk(1);
                    
                    if (i < C_SIZE)
                    {
                        E_STEP3_prev_k = sl_tr_result_i->read();
                        cstep = E_STEP3;
                    }
                    else
                    {
                        E_STEP2_prev_k = sl_tr_result_i->read();
                        cstep = E_STEP2;
                    }

                    break;
                }
            case E_STEP1:
                {
                    const u512 &k = E_STEP1_k;
                    const u512 &m = E_STEP1_m;

                    DEBUG_OUT << "E_STEP1 " << "m = " << m.to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "E_STEP1 " << "k = " << k.to_string(sc_dt::SC_HEX) << std::endl;

                    p_tr_a_s_.write(m ^ k);
                    wait_clk(1);

                    sl_tr_a_s_.write(p_tr_result_i->read());
                    wait_clk(1);

                    E_STEP3_new_m = sl_tr_result_i->read();
                    E_STEP2_prev_k = k;

                    cstep = E_STEP2;
                    break;
                }
            case E_STEP2:
                {
                    const u512 &prev_k = E_STEP2_prev_k;

                    DEBUG_OUT << "E_STEP2 " << "prev_k = " << prev_k.to_string(sc_dt::SC_HEX) << std::endl;

                    K_I_i = K_I_i + 1;
                    K_I_prev_k = prev_k;

                    if (K_I_i <= C_SIZE)
                    {
                        cstep = K_I;
                    }
                    else
                    {
                        E_STEP4_prev_k = prev_k;
                        cstep = E_STEP4;
                    }
                    break;
                }
            case E_STEP3:
                {
                    const u512 &new_m = E_STEP3_new_m;
                    const u512 &prev_k = E_STEP3_prev_k;
                    
                    DEBUG_OUT << "E_STEP3 " << "new_m = " << new_m.to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "E_STEP3 " << "prev_k = " << prev_k.to_string(sc_dt::SC_HEX) << std::endl;

                    p_tr_a_s_.write(new_m ^ prev_k);
                    wait_clk(1);

                    sl_tr_a_s_.write(p_tr_result_i->read());
                    wait_clk(1);

                    E_STEP3_new_m = sl_tr_result_i->read();
                    E_STEP2_prev_k = prev_k;

                    cstep = E_STEP2;
                    break;
                }
            case E_STEP4:
                {
                    const u512 &new_m = E_STEP3_new_m;
                    const u512 &prev_k = E_STEP4_prev_k;

                    DEBUG_OUT << "E_STEP4 " << "new_m = " << new_m.to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "E_STEP4 " << "prev_k = " << prev_k.to_string(sc_dt::SC_HEX) << std::endl;

                    G_N_STEP2_E = new_m ^ prev_k;

                    cstep = G_N_STEP2;
                    break;
                }
            case G_N_STEP1:
                {
                    const u512 &h = h_i->read();
                    const u512 &m = m_i->read();
                    const u512 &n = n_i->read();

                    DEBUG_OUT << "G_N_STEP1 " << "h = " << h.to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "G_N_STEP1 " << "m = " << m.to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "G_N_STEP1 " << "n = " << n.to_string(sc_dt::SC_HEX) << std::endl;

                    p_tr_a_s_.write(h ^ n);
                    wait_clk(1);

                    sl_tr_a_s_.write(p_tr_result_i->read());
                    wait_clk(1);

                    E_STEP1_k = sl_tr_result_i->read();
                    E_STEP1_m = m;

                    cstep = E_STEP1;
                    break;
                }
            case G_N_STEP2:
                {
                    const u512 &h = h_i->read();
                    const u512 &m = m_i->read();
                    const u512 &e = G_N_STEP2_E;

                    DEBUG_OUT << "G_N_STEP2 " << "h = " << h.to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "G_N_STEP2 " << "m = " << m.to_string(sc_dt::SC_HEX) << std::endl;
                    DEBUG_OUT << "G_N_STEP2 " << "e = " << e.to_string(sc_dt::SC_HEX) << std::endl;

                    result = e ^ h ^ m;

                    DEBUG_OUT << "G_N_STEP2 " << "result = " << result.to_string(sc_dt::SC_HEX) << std::endl;
                    finished = true;
                    break;
                }
        }
    }

    return result;
}

const uint64_t sl_precomp_table[8][256] = {
    {
        0xd01f715b5c7ef8e6, 0x16fa240980778325, 0xa8a42e857ee049c8, 0x6ac1068fa186465b,
        0x6e417bd7a2e9320b, 0x665c8167a437daab, 0x7666681aa89617f6, 0x4b959163700bdcf5,
        0xf14be6b78df36248, 0xc585bd689a625cff, 0x9557d7fca67d82cb, 0x89f0b969af6dd366,
        0xb0833d48749f6c35, 0xa1998c23b1ecbc7c, 0x8d70c431ac02a736, 0xd6dfbc2fd0a8b69e,
        0x37aeb3e551fa198b, 0x0b7d128a40b5cf9c, 0x5a8f2008b5780cbc, 0xedec882284e333e5,
        0xd25fc177d3c7c2ce, 0x5e0f5d50b61778ec, 0x1d873683c0c24cb9, 0xad040bcbb45d208c,
        0x2f89a0285b853c76, 0x5732fff6791b8d58, 0x3e9311439ef6ec3f, 0xc9183a809fd3c00f,
        0x83adf3f5260a01ee, 0xa6791941f4e8ef10, 0x103ae97d0ca1cd5d, 0x2ce948121dee1b4a,
        0x39738421dbf2bf53, 0x093da2a6cf0cf5b4, 0xcd9847d89cbcb45f, 0xf9561c078b2d8ae8,
        0x9c6a755a6971777f, 0xbc1ebaa0712ef0c5, 0x72e61542abf963a6, 0x78bb5fde229eb12e,
        0x14ba94250fceb90d, 0x844d6697630e5282, 0x98ea08026a1e032f, 0xf06bbea144217f5c,
        0xdb6263d11ccb377a, 0x641c314b2b8ee083, 0x320e96ab9b4770cf, 0x1ee7deb986a96b85,
        0xe96cf57a878c47b5, 0xfdd6615f8842feb8, 0xc83862965601dd1b, 0x2ea9f83e92572162,
        0xf876441142ff97fc, 0xeb2c455608357d9d, 0x5612a7e0b0c9904c, 0x6c01cbfb2d500823,
        0x4548a6a7fa037a2d, 0xabc4c6bf388b6ef4, 0xbade77d4fdf8bebd, 0x799b07c8eb4cac3a,
        0x0c9d87e805b19cf0, 0xcb588aac106afa27, 0xea0c1d40c1e76089, 0x2869354a1e816f1a,
        0xff96d17307fbc490, 0x9f0a9d602f1a5043, 0x96373fc6e016a5f7, 0x5292dab8b3a6e41c,
        0x9b8ae0382c752413, 0x4f15ec3b7364a8a5, 0x3fb349555724f12b, 0xc7c50d4415db66d7,
        0x92b7429ee379d1a7, 0xd37f99611a15dfda, 0x231427c05e34a086, 0xa439a96d7b51d538,
        0xb403401077f01865, 0xdda2aea5901d7902, 0x0a5d4a9c8967d288, 0xc265280adf660f93,
        0x8bb0094520d4e94e, 0x2a29856691385532, 0x42a833c5bf072941, 0x73c64d54622b7eb2,
        0x07e095624504536c, 0x8a905153e906f45a, 0x6f6123c16b3b2f1f, 0xc6e55552dc097bc3,
        0x4468feb133d16739, 0xe211e7f0c7398829, 0xa2f96419f7879b40, 0x19074bdbc3ad38e9,
        0xf4ebc3f9474e0b0c, 0x43886bd376d53455, 0xd8028beb5aa01046, 0x51f23282f5cdc320,
        0xe7b1c2be0d84e16d, 0x081dfab006dee8a0, 0x3b33340d544b857b, 0x7f5bcabc679ae242,
        0x0edd37c48a08a6d8, 0x81ed43d9a9b33bc6, 0xb1a3655ebd4d7121, 0x69a1eeb5e7ed6167,
        0xf6ab73d5c8f73124, 0x1a67a3e185c61fd5, 0x2dc91004d43c065e, 0x0240b02c8fb93a28,
        0x90f7f2b26cc0eb8f, 0x3cd3a16f114fd617, 0xaae49ea9f15973e0, 0x06c0cd748cd64e78,
        0xda423bc7d5192a6e, 0xc345701c16b41287, 0x6d2193ede4821537, 0xfcf639494190e3ac,
        0x7c3b228621f1c57e, 0xfb16ac2b0494b0c0, 0xbf7e529a3745d7f9, 0x6881b6a32e3f7c73,
        0xca78d2bad9b8e733, 0xbbfe2fc2342aa3a9, 0x0dbddffecc6381e4, 0x70a6a56e2440598e,
        0xe4d12a844befc651, 0x8c509c2765d0ba22, 0xee8c6018c28814d9, 0x17da7c1f49a59e31,
        0x609c4c1328e194d3, 0xb3e3d57232f44b09, 0x91d7aaa4a512f69b, 0x0ffd6fd243dabbcc,
        0x50d26a943c1fde34, 0x6be15e9968545b4f, 0x94778fea6faf9fdf, 0x2b09dd7058ea4826,
        0x677cd9716de5c7bf, 0x49d5214fffb2e6dd, 0x0360e83a466b273c, 0x1fc786af4f7b7691,
        0xa0b9d435783ea168, 0xd49f0c035f118cb6, 0x01205816c9d21d14, 0xac2453dd7d8f3d98,
        0x545217cc3f70aa64, 0x26b4028e9489c9c2, 0xdec2469fd6765e3e, 0x04807d58036f7450,
        0xe5f17292823ddb45, 0xf30b569b024a5860, 0x62dcfc3fa758aefb, 0xe84cad6c4e5e5aa1,
        0xccb81fce556ea94b, 0x53b282ae7a74f908, 0x1b47fbf74c1402c1, 0x368eebf39828049f,
        0x7afbeff2ad278b06, 0xbe5e0a8cfe97caed, 0xcfd8f7f413058e77, 0xf78b2bc301252c30,
        0x4d555c17fcdd928d, 0x5f2f05467fc565f8, 0x24f4b2a21b30f3ea, 0x860dd6bbecb768aa,
        0x4c750401350f8f99, 0x0000000000000000, 0xecccd0344d312ef1, 0xb5231806be220571,
        0xc105c030990d28af, 0x653c695de25cfd97, 0x159acc33c61ca419, 0xb89ec7f872418495,
        0xa9847693b73254dc, 0x58cf90243ac13694, 0x59efc832f3132b80, 0x5c4fed7c39ae42c4,
        0x828dabe3efd81cfa, 0xd13f294d95ace5f2, 0x7d1b7a90e823d86a, 0xb643f03cf849224d,
        0x3df3f979d89dcb03, 0x7426d836272f2dde, 0xdfe21e891fa4432a, 0x3a136c1b9d99986f,
        0xfa36f43dcd46add4, 0xc025982650df35bb, 0x856d3e81aadc4f96, 0xc4a5e57e53b041eb,
        0x4708168b75ba4005, 0xaf44bbe73be41aa4, 0x971767d029c4b8e3, 0xb9be9feebb939981,
        0x215497ecd18d9aae, 0x316e7e91dd2c57f3, 0xcef8afe2dad79363, 0x3853dc371220a247,
        0x35ee03c9de4323a3, 0xe6919aa8c456fc79, 0xe05157dc4880b201, 0x7bdbb7e464f59612,
        0x127a59518318f775, 0x332ecebd52956ddb, 0x8f30741d23bb9d1e, 0xd922d3fd93720d52,
        0x7746300c61440ae2, 0x25d4eab4d2e2eefe, 0x75068020eefd30ca, 0x135a01474acaea61,
        0x304e268714fe4ae7, 0xa519f17bb283c82c, 0xdc82f6b359cf6416, 0x5baf781e7caa11a8,
        0xb2c38d64fb26561d, 0x34ce5bdf17913eb7, 0x5d6fb56af07c5fd0, 0x182713cd0a7f25fd,
        0x9e2ac576e6c84d57, 0x9aaab82ee5a73907, 0xa3d93c0f3e558654, 0x7e7b92aaae48ff56,
        0x872d8ead256575be, 0x41c8dbfff96c0e7d, 0x99ca5014a3cc1e3b, 0x40e883e930be1369,
        0x1ca76e95091051ad, 0x4e35b42dbab6b5b1, 0x05a0254ecabd6944, 0xe1710fca8152af15,
        0xf22b0e8dcb984574, 0xb763a82a319b3f59, 0x63fca4296e8ab3ef, 0x9d4a2d4ca0a36a6b,
        0xe331bfe60eeb953d, 0xd5bf541596c391a2, 0xf5cb9bef8e9c1618, 0x46284e9dbc685d11,
        0x2074cffa185f87ba, 0xbd3ee2b6b8fcedd1, 0xae64e3f1f23607b0, 0xfeb68965ce29d984,
        0x55724fdaf6a2b770, 0x29496d5cd753720e, 0xa75941573d3af204, 0x8e102c0bea69800a,
        0x111ab16bc573d049, 0xd7ffe439197aab8a, 0xefac380e0b5a09cd, 0x48f579593660fbc9,
        0x22347fd697e6bd92, 0x61bc1405e13389c7, 0x4ab5c975b9d9c1e1, 0x80cd1bcf606126d2,
        0x7186fd78ed92449a, 0x93971a882aabccb3, 0x88d0e17f66bfce72, 0x27945a985d5bd4d6,
    },
    {
        0xde553f8c05a811c8, 0x1906b59631b4f565, 0x436e70d6b1964ff7, 0x36d343cb8b1e9d85,
        0x843dfacc858aab5a, 0xfdfc95c299bfc7f9, 0x0f634bdea1d51fa2, 0x6d458b3b76efb3cd,
        0x85c3f77cf8593f80, 0x3c91315fbe737cb2, 0x2148b03366ace398, 0x18f8b8264c6761bf,
        0xc830c1c495c9fb0f, 0x981a76102086a0aa, 0xaa16012142f35760, 0x35cc54060c763cf6,
        0x42907d66cc45db2d, 0x8203d44b965af4bc, 0x3d6f3cefc3a0e868, 0xbc73ff69d292bda7,
        0x8722ed0102e20a29, 0x8f8185e8cd34deb7, 0x9b0561dda7ee01d9, 0x5335a0193227fad6,
        0xc9cecc74e81a6fd5, 0x54f5832e5c2431ea, 0x99e47ba05d553470, 0xf7bee756acd226ce,
        0x384e05a5571816fd, 0xd1367452a47d0e6a, 0xf29fde1c386ad85b, 0x320c77316275f7ca,
        0xd0c879e2d9ae9ab0, 0xdb7406c69110ef5d, 0x45505e51a2461011, 0xfc029872e46c5323,
        0xfa3cb6f5f7bc0cc5, 0x031f17cd8768a173, 0xbd8df2d9af41297d, 0x9d3b4f5ab43e5e3f,
        0x4071671b36feee84, 0x716207e7d3e3b83d, 0x48d20ff2f9283a1a, 0x27769eb4757cbc7e,
        0x5c56ebc793f2e574, 0xa48b474f9ef5dc18, 0x52cbada94ff46e0c, 0x60c7da982d8199c6,
        0x0e9d466edc068b78, 0x4eec2175eaf865fc, 0x550b8e9e21f7a530, 0x6b7ba5bc653fec2b,
        0x5eb7f1ba6949d0dd, 0x57ea94e3db4c9099, 0xf640eae6d101b214, 0xdd4a284182c0b0bb,
        0xff1d8fbf6304f250, 0xb8accb933bf9d7e8, 0xe8867c478eb68c4d, 0x3f8e2692391bddc1,
        0xcb2fd60912a15a7c, 0xaec935dbab983d2f, 0xf55ffd2b56691367, 0x80e2ce366ce1c115,
        0x179bf3f8edb27e1d, 0x01fe0db07dd394da, 0xda8a0b76ecc37b87, 0x44ae53e1df9584cb,
        0xb310b4b77347a205, 0xdfab323c787b8512, 0x3b511268d070b78e, 0x65e6e3d2b9396753,
        0x6864b271e2574d58, 0x259784c98fc789d7, 0x02e11a7dfabb35a9, 0x8841a6dfa337158b,
        0x7ade78c39b5dcdd0, 0xb7cf804d9a2cc84a, 0x20b6bd831b7f7742, 0x75bd331d3a88d272,
        0x418f6aab4b2d7a5e, 0xd9951cbb6babdaf4, 0xb6318dfde7ff5c90, 0x1f389b112264aa83,
        0x492c024284fbaec0, 0xe33a0363c608f9a0, 0x2688930408af28a4, 0xc7538a1a341ce4ad,
        0x5da8e677ee2171ae, 0x8c9e92254a5c7fc4, 0x63d8cd55aae938b5, 0x29ebd8daa97a3706,
        0x959827b37be88aa1, 0x1484e4356adadf6e, 0xa7945082199d7d6b, 0xbf6ce8a455fa1cd4,
        0x9cc542eac9edcae5, 0x79c16f0e1c356ca3, 0x89bfab6fdee48151, 0xd4174d1830c5f0ff,
        0x9258048415eb419d, 0x6139d72850520d1c, 0x6a85a80c18ec78f1, 0xcd11f88e0171059a,
        0xcceff53e7ca29140, 0xd229639f2315af19, 0x90b91ef9ef507434, 0x5977d28d074a1be1,
        0x311360fce51d56b9, 0xc093a92d5a1f2f91, 0x1a19a25bb6dc5416, 0xeb996b8a09de2d3e,
        0xfee3820f1ed7668a, 0xd7085ad5b7ad518c, 0x7fff41890fe53345, 0xec5948bd67dde602,
        0x2fd5f65dbaaa68e0, 0xa5754affe32648c2, 0xf8ddac880d07396c, 0x6fa491468c548664,
        0x0c7c5c1326bdbed1, 0x4a33158f03930fb3, 0x699abfc19f84d982, 0xe4fa2054a80b329c,
        0x6707f9af438252fa, 0x08a368e9cfd6d49e, 0x47b1442c58fd25b8, 0xbbb3dc5ebc91769b,
        0x1665fe489061eac7, 0x33f27a811fa66310, 0x93a609346838d547, 0x30ed6d4c98cec263,
        0x1dd9816cd8df9f2a, 0x94662a03063b1e7b, 0x83fdd9fbeb896066, 0x7b207573e68e590a,
        0x5f49fc0a149a4407, 0x343259b671a5a82c, 0xfbc2bb458a6f981f, 0xc272b350a0a41a38,
        0x3aaf1fd8ada32354, 0x6cbb868b0b3c2717, 0xa2b569c88d2583fe, 0xf180c9d1bf027928,
        0xaf37386bd64ba9f5, 0x12bacab2790a8088, 0x4c0d3b0810435055, 0xb2eeb9070e9436df,
        0xc5b29067cea7d104, 0xdcb425f1ff132461, 0x4f122cc5972bf126, 0xac282fa651230886,
        0xe7e537992f6393ef, 0xe61b3a2952b00735, 0x709c0a57ae302ce7, 0xe02514ae416058d3,
        0xc44c9dd7b37445de, 0x5a68c5408022ba92, 0x1c278cdca50c0bf0, 0x6e5a9cf6f18712be,
        0x86dce0b17f319ef3, 0x2d34ec2040115d49, 0x4bcd183f7e409b69, 0x2815d56ad4a9a3dc,
        0x24698979f2141d0d, 0x0000000000000000, 0x1ec696a15fb73e59, 0xd86b110b16784e2e,
        0x8e7f8858b0e74a6d, 0x063e2e8713d05fe6, 0xe2c40ed3bbdb6d7a, 0xb1f1aeca89fc97ac,
        0xe1db191e3cb3cc09, 0x6418ee62c4eaf389, 0xc6ad87aa49cf7077, 0xd6f65765ca7ec556,
        0x9afb6c6dda3d9503, 0x7ce05644888d9236, 0x8d609f95378feb1e, 0x23a9aa4e9c17d631,
        0x6226c0e5d73aac6f, 0x56149953a69f0443, 0xeeb852c09d66d3ab, 0x2b0ac2a753c102af,
        0x07c023376e03cb3c, 0x2ccae1903dc2c993, 0xd3d76e2f5ec63bc3, 0x9e2458973356ff4c,
        0xa66a5d32644ee9b1, 0x0a427294356de137, 0x783f62be61e6f879, 0x1344c70204d91452,
        0x5b96c8f0fdf12e48, 0xa90916ecc59bf613, 0xbe92e5142829880e, 0x727d102a548b194e,
        0x1be7afebcb0fc0cc, 0x3e702b2244c8491b, 0xd5e940a84d166425, 0x66f9f41f3e51c620,
        0xabe80c913f20c3ba, 0xf07ec461c2d1edf2, 0xf361d3ac45b94c81, 0x0521394a94b8fe95,
        0xadd622162cf09c5c, 0xe97871f7f3651897, 0xf4a1f09b2bba87bd, 0x095d6559b2054044,
        0x0bbc7f2448be75ed, 0x2af4cf172e129675, 0x157ae98517094bb4, 0x9fda55274e856b96,
        0x914713499283e0ee, 0xb952c623462a4332, 0x74433ead475b46a8, 0x8b5eb112245fb4f8,
        0xa34b6478f0f61724, 0x11a5dd7ffe6221fb, 0xc16da49d27ccbb4b, 0x76a224d0bde07301,
        0x8aa0bca2598c2022, 0x4df336b86d90c48f, 0xea67663a740db9e4, 0xef465f70e0b54771,
        0x39b008152acb8227, 0x7d1e5bf4f55e06ec, 0x105bd0cf83b1b521, 0x775c2960c033e7db,
        0x7e014c397236a79f, 0x811cc386113255cf, 0xeda7450d1a0e72d8, 0x5889df3d7a998f3b,
        0x2e2bfbedc779fc3a, 0xce0eef438619a4e9, 0x372d4e7bf6cd095f, 0x04df34fae96b6a4f,
        0xf923a13870d4adb6, 0xa1aa7e050a4d228d, 0xa8f71b5cb84862c9, 0xb52e9a306097fde3,
        0x0d8251a35b6e2a0b, 0x2257a7fee1c442eb, 0x73831d9a29588d94, 0x51d4ba64c89ccf7f,
        0x502ab7d4b54f5ba5, 0x97793dce8153bf08, 0xe5042de4d5d8a646, 0x9687307efc802bd2,
        0xa05473b5779eb657, 0xb4d097801d446939, 0xcff0e2f3fbca3033, 0xc38cbee0dd778ee2,
        0x464f499c252eb162, 0xcad1dbb96f72cea6, 0xba4dd1eec142e241, 0xb00fa37af42f0376,
    },
    {
        0xcce4cd3aa968b245, 0x089d5484e80b7faf, 0x638246c1b3548304, 0xd2fe0ec8c2355492,
        0xa7fbdf7ff2374eee, 0x4df1600c92337a16, 0x84e503ea523b12fb, 0x0790bbfd53ab0c4a,
        0x198a780f38f6ea9d, 0x2ab30c8f55ec48cb, 0xe0f7fed6b2c49db5, 0xb6ecf3f422cadbdc,
        0x409c9a541358df11, 0xd3ce8a56dfde3fe3, 0xc3e9224312c8c1a0, 0x0d6dfa58816ba507,
        0xddf3e1b179952777, 0x04c02a42748bb1d9, 0x94c2abff9f2decb8, 0x4f91752da8f8acf4,
        0x78682befb169bf7b, 0xe1c77a48af2ff6c4, 0x0c5d7ec69c80ce76, 0x4cc1e4928fd81167,
        0xfeed3d24d9997b62, 0x518bb6dfc3a54a23, 0x6dbf2d26151f9b90, 0xb5bc624b05ea664f,
        0xe86aaa525acfe21a, 0x4801ced0fb53a0be, 0xc91463e6c00868ed, 0x1027a815cd16fe43,
        0xf67069a0319204cd, 0xb04ccc976c8abce7, 0xc0b9b3fc35e87c33, 0xf380c77c58f2de65,
        0x50bb3241de4e2152, 0xdf93f490435ef195, 0xf1e0d25d62390887, 0xaf668bfb1a3c3141,
        0xbc11b251f00a7291, 0x73a5eed47e427d47, 0x25bee3f6ee4c3b2e, 0x43cc0beb34786282,
        0xc824e778dde3039c, 0xf97d86d98a327728, 0xf2b043e24519b514, 0xe297ebf7880f4b57,
        0x3a94a49a98fab688, 0x868516cb68f0c419, 0xeffa11af0964ee50, 0xa4ab4ec0d517f37d,
        0xa9c6b498547c567a, 0x8e18424f80fbbbb6, 0x0bcdc53bcf2bc23c, 0x137739aaea3643d0,
        0x2c1333ec1bac2ff0, 0x8d48d3f0a7db0625, 0x1e1ac3f26b5de6d7, 0xf520f81f16b2b95e,
        0x9f0f6ec450062e84, 0x0130849e1deb6b71, 0xd45e31ab8c7533a9, 0x652279a2fd14e43f,
        0x3209f01e70f1c927, 0xbe71a770cac1a473, 0x0e3d6be7a64b1894, 0x7ec8148cff29d840,
        0xcb7476c7fac3be0f, 0x72956a4a63a91636, 0x37f95ec21991138f, 0x9e3fea5a4ded45f5,
        0x7b38ba50964902e8, 0x222e580bbde73764, 0x61e253e0899f55e6, 0xfc8d2805e352ad80,
        0x35994be3235ac56d, 0x09add01af5e014de, 0x5e8659a6780539c6, 0xb17c48097161d796,
        0x026015213acbd6e2, 0xd1ae9f77e515e901, 0xb7dc776a3f21b0ad, 0xaba6a1b96eb78098,
        0x9bcf4486248d9f5d, 0x582666c536455efd, 0xfdbdac9bfeb9c6f1, 0xc47999be4163cdea,
        0x765540081722a7ef, 0x3e548ed8ec710751, 0x3d041f67cb51bac2, 0x7958af71ac82d40a,
        0x36c9da5c047a78fe, 0xed9a048e33af38b2, 0x26ee7249c96c86bd, 0x900281bdeba65d61,
        0x11172c8bd0fd9532, 0xea0abf73600434f8, 0x42fc8f75299309f3, 0x34a9cf7d3eb1ae1c,
        0x2b838811480723ba, 0x5ce64c8742ceef24, 0x1adae9b01fd6570e, 0x3c349bf9d6bad1b3,
        0x82453c891c7b75c0, 0x97923a40b80d512b, 0x4a61dbf1c198765c, 0xb48ce6d518010d3e,
        0xcfb45c858e480fd6, 0xd933cbf30d1e96ae, 0xd70ea014ab558e3a, 0xc189376228031742,
        0x9262949cd16d8b83, 0xeb3a3bed7def5f89, 0x49314a4ee6b8cbcf, 0xdcc3652f647e4c06,
        0xda635a4c2a3e2b3d, 0x470c21a940f3d35b, 0x315961a157d174b4, 0x6672e81dda3459ac,
        0x5b76f77a1165e36e, 0x445cb01667d36ec8, 0xc5491d205c88a69b, 0x456c34887a3805b9,
        0xffddb9bac4721013, 0x99af51a71e4649bf, 0xa15be01cbc7729d5, 0x52db2760e485f7b0,
        0x8c78576eba306d54, 0xae560f6507d75a30, 0x95f22f6182c687c9, 0x71c5fbf54489aba5,
        0xca44f259e728d57e, 0x88b87d2ccebbdc8d, 0xbab18d32be4a15aa, 0x8be8ec93e99b611e,
        0x17b713e89ebdf209, 0xb31c5d284baa0174, 0xeeca9531148f8521, 0xb8d198138481c348,
        0x8988f9b2d350b7fc, 0xb9e11c8d996aa839, 0x5a4673e40c8e881f, 0x1687977683569978,
        0xbf4123eed72acf02, 0x4ea1f1b3b513c785, 0xe767452be16f91ff, 0x7505d1b730021a7c,
        0xa59bca5ec8fc980c, 0xad069eda20f7e7a3, 0x38f4b1bba231606a, 0x60d2d77e94743e97,
        0x9affc0183966f42c, 0x248e6768f3a7505f, 0xcdd449a4b483d934, 0x87b59255751baf68,
        0x1bea6d2e023d3c7f, 0x6b1f12455b5ffcab, 0x743555292de9710d, 0xd8034f6d10f5fddf,
        0xc6198c9f7ba81b08, 0xbb8109aca3a17edb, 0xfa2d1766ad12cabb, 0xc729080166437079,
        0x9c5fff7b77269317, 0x0000000000000000, 0x15d706c9a47624eb, 0x6fdf38072fd44d72,
        0x5fb6dd3865ee52b7, 0xa33bf53d86bcff37, 0xe657c1b5fc84fa8e, 0xaa962527735cebe9,
        0x39c43525bfda0b1b, 0x204e4d2a872ce186, 0x7a083ece8ba26999, 0x554b9c9db72efbfa,
        0xb22cd9b656416a05, 0x96a2bedea5e63a5a, 0x802529a826b0a322, 0x8115ad363b5bc853,
        0x8375b81701901eb1, 0x3069e53f4a3a1fc5, 0xbd2136cfede119e0, 0x18bafc91251d81ec,
        0x1d4a524d4c7d5b44, 0x05f0aedc6960daa8, 0x29e39d3072ccf558, 0x70f57f6b5962c0d4,
        0x989fd53903ad22ce, 0xf84d024797d91c59, 0x547b1803aac5908b, 0xf0d056c37fd263f6,
        0xd56eb535919e58d8, 0x1c7ad6d351963035, 0x2e7326cd2167f912, 0xac361a443d1c8cd2,
        0x697f076461942a49, 0x4b515f6fdc731d2d, 0x8ad8680df4700a6f, 0x41ac1eca0eb3b460,
        0x7d988533d80965d3, 0xa8f6300649973d0b, 0x7765c4960ac9cc9e, 0x7ca801adc5e20ea2,
        0xdea3700e5eb59ae4, 0xa06b6482a19c42a4, 0x6a2f96db46b497da, 0x27def6d7d487edcc,
        0x463ca5375d18b82a, 0xa6cb5be1efdc259f, 0x53eba3fef96e9cc1, 0xce84d81b93a364a7,
        0xf4107c810b59d22f, 0x333974806d1aa256, 0x0f0def79bba073e5, 0x231edc95a00c5c15,
        0xe437d494c64f2c6c, 0x91320523f64d3610, 0x67426c83c7df32dd, 0x6eefbc99323f2603,
        0x9d6f7be56acdf866, 0x5916e25b2bae358c, 0x7ff89012e2c2b331, 0x035091bf2720bd93,
        0x561b0d22900e4669, 0x28d319ae6f279e29, 0x2f43a2533c8c9263, 0xd09e1be9f8fe8270,
        0xf740ed3e2c796fbc, 0xdb53ded237d5404c, 0x62b2c25faebfe875, 0x0afd41a5d2c0a94d,
        0x6412fd3ce0ff8f4e, 0xe3a76f6995e42026, 0x6c8fa9b808f4f0e1, 0xc2d9a6dd0f23aad1,
        0x8f28c6d19d10d0c7, 0x85d587744fd0798a, 0xa20b71a39b579446, 0x684f83fa7c7f4138,
        0xe507500adba4471d, 0x3f640a46f19a6c20, 0x1247bd34f7dd28a1, 0x2d23b77206474481,
        0x93521002cc86e0f2, 0x572b89bc8de52d18, 0xfb1d93f8b0f9a1ca, 0xe95a2ecc4724896b,
        0x3ba420048511ddf9, 0xd63e248ab6bee54b, 0x5dd6c8195f258455, 0x06a03f634e40673b,
        0x1f2a476c76b68da6, 0x217ec9b49ac78af7, 0xecaa80102e4453c3, 0x14e78257b99d4f9a,
    },
    {
        0x20329b2cc87bba05, 0x4f5eb6f86546a531, 0xd4f44775f751b6b1, 0x8266a47b850dfa8b,
        0xbb986aa15a6ca985, 0xc979eb08f9ae0f99, 0x2da6f447a2375ea1, 0x1e74275dcd7d8576,
        0xbc20180a800bc5f8, 0xb4a2f701b2dc65be, 0xe726946f981b6d66, 0x48e6c453bf21c94c,
        0x42cad9930f0a4195, 0xefa47b64aacccd20, 0x71180a8960409a42, 0x8bb3329bf6a44e0c,
        0xd34c35de2d36dacc, 0xa92f5b7cbc23dc96, 0xb31a85aa68bb09c3, 0x13e04836a73161d2,
        0xb24dfc4129c51d02, 0x8ae44b70b7da5acd, 0xe671ed84d96579a7, 0xa4bb3417d66f3832,
        0x4572ab38d56d2de8, 0xb1b47761ea47215c, 0xe81c09cf70aba15d, 0xffbdb872ce7f90ac,
        0xa8782297fd5dc857, 0x0d946f6b6a4ce4a4, 0xe4df1f4f5b995138, 0x9ebc71edca8c5762,
        0x0a2c1dc0b02b88d9, 0x3b503c115d9d7b91, 0xc64376a8111ec3a2, 0xcec199a323c963e4,
        0xdc76a87ec58616f7, 0x09d596e073a9b487, 0x14583a9d7d560daf, 0xf4c6dc593f2a0cb4,
        0xdd21d19584f80236, 0x4a4836983ddde1d3, 0xe58866a41ae745f9, 0xf591a5b27e541875,
        0x891dc05074586693, 0x5b068c651810a89e, 0xa30346bc0c08544f, 0x3dbf3751c684032d,
        0x2a1e86ec785032dc, 0xf73f5779fca830ea, 0xb60c05ca30204d21, 0x0cc316802b32f065,
        0x8770241bdd96be69, 0xb861e18199ee95db, 0xf805cad91418fcd1, 0x29e70dccbbd20e82,
        0xc7140f435060d763, 0x0f3a9da0e8b0cc3b, 0xa2543f574d76408e, 0xbd7761e1c175d139,
        0x4b1f4f737ca3f512, 0x6dc2df1f2fc137ab, 0xf1d05c3967b14856, 0xa742bf3715ed046c,
        0x654030141d1697ed, 0x07b872abda676c7d, 0x3ce84eba87fa17ec, 0xc1fb0403cb79afdf,
        0x3e46bc7105063f73, 0x278ae987121cd678, 0xa1adb4778ef47cd0, 0x26dd906c5362c2b9,
        0x05168060589b44e2, 0xfbfc41f9d79ac08f, 0x0e6de44ba9ced8fa, 0x9feb08068bf243a3,
        0x7b341749d06b129b, 0x229c69e74a87929a, 0xe09ee6c4427c011b, 0x5692e30e725c4c3a,
        0xda99a33e5e9f6e4b, 0x353dd85af453a36b, 0x25241b4c90e0fee7, 0x5de987258309d022,
        0xe230140fc0802984, 0x93281e86a0c0b3c6, 0xf229d719a4337408, 0x6f6c2dd4ad3d1f34,
        0x8ea5b2fbae3f0aee, 0x8331dd90c473ee4a, 0x346aa1b1b52db7aa, 0xdf8f235e06042aa9,
        0xcc6f6b68a1354b7b, 0x6c95a6f46ebf236a, 0x52d31a856bb91c19, 0x1a35ded6d498d555,
        0xf37eaef2e54d60c9, 0x72e181a9a3c2a61c, 0x98537aad51952fde, 0x16f6c856ffaa2530,
        0xd960281e9d1d5215, 0x3a0745fa1ce36f50, 0x0b7b642bf1559c18, 0x59a87eae9aec8001,
        0x5e100c05408bec7c, 0x0441f98b19e55023, 0xd70dcc5534d38aef, 0x927f676de1bea707,
        0x9769e70db925e3e5, 0x7a636ea29115065a, 0x468b201816ef11b6, 0xab81a9b73edff409,
        0xc0ac7de88a07bb1e, 0x1f235eb68c0391b7, 0x6056b074458dd30f, 0xbe8eeac102f7ed67,
        0xcd381283e04b5fba, 0x5cbefecec277c4e3, 0xd21b4c356c48ce0d, 0x1019c31664b35d8c,
        0x247362a7d19eea26, 0xebe582efb3299d03, 0x02aef2cb82fc289f, 0x86275df09ce8aaa8,
        0x28b07427faac1a43, 0x38a9b7319e1f47cf, 0xc82e92e3b8d01b58, 0x06ef0b409b1978bc,
        0x62f842bfc771fb90, 0x9904034610eb3b1f, 0xded85ab5477a3e68, 0x90d195a663428f98,
        0x5384636e2ac708d8, 0xcbd719c37b522706, 0xae9729d76644b0eb, 0x7c8c65e20a0c7ee6,
        0x80c856b007f1d214, 0x8c0b40302cc32271, 0xdbcedad51fe17a8a, 0x740e8ae938dbdea0,
        0xa615c6dc549310ad, 0x19cc55f6171ae90b, 0x49b1bdb8fe5fdd8d, 0xed0a89af2830e5bf,
        0x6a7aadb4f5a65bd6, 0x7e22972988f05679, 0xf952b3325566e810, 0x39fecedadf61530e,
        0x6101c99f04f3c7ce, 0x2e5f7f6761b562ff, 0xf08725d226cf5c97, 0x63af3b54860fef51,
        0x8ff2cb10ef411e2f, 0x884ab9bb35267252, 0x4df04433e7ba8dae, 0x9afd8866d3690741,
        0x66b9bb34de94abb3, 0x9baaf18d92171380, 0x543c11c5f0a064a5, 0x17a1b1bdbed431f1,
        0xb5f58eeaf3a2717f, 0xc355f6c849858740, 0xec5df044694ef17e, 0xd83751f5dc6346d4,
        0xfc4433520dfdacf2, 0x0000000000000000, 0x5a51f58e596ebc5f, 0x3285aaf12e34cf16,
        0x8d5c39db6dbd36b0, 0x12b731dde64f7513, 0x94906c2d7aa7dfbb, 0x302b583aacc8e789,
        0x9d45facd090e6b3c, 0x2165e2c78905aec4, 0x68d45f7f775a7349, 0x189b2c1d5664fdca,
        0xe1c99f2f030215da, 0x6983269436246788, 0x8489af3b1e148237, 0xe94b702431d5b59c,
        0x33d2d31a6f4adbd7, 0xbfd9932a4389f9a6, 0xb0e30e8aab39359d, 0xd1e2c715afcaf253,
        0x150f43763c28196e, 0xc4ed846393e2eb3d, 0x03f98b20c3823c5e, 0xfd134ab94c83b833,
        0x556b682eb1de7064, 0x36c4537a37d19f35, 0x7559f30279a5ca61, 0x799ae58252973a04,
        0x9c12832648707ffd, 0x78cd9c6913e92ec5, 0x1d8dac7d0effb928, 0x439da0784e745554,
        0x413352b3cc887dcb, 0xbacf134a1b12bd44, 0x114ebafd25cd494d, 0x2f08068c20cb763e,
        0x76a07822ba27f63f, 0xeab2fb04f25789c2, 0xe3676de481fe3d45, 0x1b62a73d95e6c194,
        0x641749ff5c68832c, 0xa5ec4dfc97112cf3, 0xf6682e92bdd6242b, 0x3f11c59a44782bb2,
        0x317c21d1edb6f348, 0xd65ab5be75ad9e2e, 0x6b2dd45fb4d84f17, 0xfaab381296e4d44e,
        0xd0b5befeeeb4e692, 0x0882ef0b32d7a046, 0x512a91a5a83b2047, 0x963e9ee6f85bf724,
        0x4e09cf132438b1f0, 0x77f701c9fb59e2fe, 0x7ddb1c094b726a27, 0x5f4775ee01f5f8bd,
        0x9186ec4d223c9b59, 0xfeeac1998f01846d, 0xac39db1ce4b89874, 0xb75b7c21715e59e0,
        0xafc0503c273aa42a, 0x6e3b543fec430bf5, 0x704f7362213e8e83, 0x58ff0745db9294c0,
        0x67eec2df9feabf72, 0xa0facd9ccf8a6811, 0xb936986ad890811a, 0x95c715c63bd9cb7a,
        0xca8060283a2c33c7, 0x507de84ee9453486, 0x85ded6d05f6a96f6, 0x1cdad5964f81ade9,
        0xd5a33e9eb62fa270, 0x40642b588df6690a, 0x7f75eec2c98e42b8, 0x2cf18dace3494a60,
        0x23cb100c0bf9865b, 0xeef3028febb2d9e1, 0x4425d2d394133929, 0xaad6d05c7fa1e0c8,
        0xad6ea2f7a5c68cb5, 0xc2028f2308fb9381, 0x819f2f5b468fc6d5, 0xc5bafd88d29cfffc,
        0x47dc59f357910577, 0x2b49ff07392e261d, 0x57c59ae5332258fb, 0x73b6f842e2bcb2dd,
        0xcf96e04862b77725, 0x4ca73dd8a6c4996f, 0x015779eb417e14c1, 0x37932a9176af8bf4,
    },
    {
        0x190a2c9b249df23e, 0x2f62f8b62263e1e9, 0x7a7f754740993655, 0x330b7ba4d5564d9f,
        0x4c17a16a46672582, 0xb22f08eb7d05f5b8, 0x535f47f40bc148cc, 0x3aec5d27d4883037,
        0x10ed0a1825438f96, 0x516101f72c233d17, 0x13cc6f949fd04eae, 0x739853c441474bfd,
        0x653793d90d3f5b1b, 0x5240647b96b0fc2f, 0x0c84890ad27623e0, 0xd7189b32703aaea3,
        0x2685de3523bd9c41, 0x99317c5b11bffefa, 0x0d9baa854f079703, 0x70b93648fbd48ac5,
        0xa80441fce30bc6be, 0x7287704bdc36ff1e, 0xb65384ed33dc1f13, 0xd36417343ee34408,
        0x39cd38ab6e1bf10f, 0x5ab861770a1f3564, 0x0ebacf09f594563b, 0xd04572b884708530,
        0x3cae9722bdb3af47, 0x4a556b6f2f5cbaf2, 0xe1704f1f76c4bd74, 0x5ec4ed7144c6dfcf,
        0x16afc01d4c7810e6, 0x283f113cd629ca7a, 0xaf59a8761741ed2d, 0xeed5a3991e215fac,
        0x3bf37ea849f984d4, 0xe413e096a56ce33c, 0x2c439d3a98f020d1, 0x637559dc6404c46b,
        0x9e6c95d1e5f5d569, 0x24bb9836045fe99a, 0x44efa466dac8ecc9, 0xc6eab2a5c80895d6,
        0x803b50c035220cc4, 0x0321658cba93c138, 0x8f9ebc465dc7ee1c, 0xd15a5137190131d3,
        0x0fa5ec8668e5e2d8, 0x91c979578d1037b1, 0x0642ca05693b9f70, 0xefca80168350eb4f,
        0x38d21b24f36a45ec, 0xbeab81e1af73d658, 0x8cbfd9cae7542f24, 0xfd19cc0d81f11102,
        0x0ac6430fbb4dbc90, 0x1d76a09d6a441895, 0x2a01573ff1cbbfa1, 0xb572e161894fde2b,
        0x8124734fa853b827, 0x614b1fdf43e6b1b0, 0x68ac395c4238cc18, 0x21d837bfd7f7b7d2,
        0x20c714304a860331, 0x5cfaab726324aa14, 0x74c5ba4eb50d606e, 0xf3a3030474654739,
        0x23e671bcf015c209, 0x45f087e947b9582a, 0xd8bd77b418df4c7b, 0xe06f6c90ebb50997,
        0x0bd96080263c0873, 0x7e03f9410e40dcfe, 0xb8e94be4c6484928, 0xfb5b0608e8ca8e72,
        0x1a2b49179e0e3306, 0x4e29e76961855059, 0x4f36c4e6fcf4e4ba, 0x49740ee395cf7bca,
        0xc2963ea386d17f7d, 0x90d65ad810618352, 0x12d34c1b02a1fa4d, 0xfa44258775bb3a91,
        0x18150f14b9ec46dd, 0x1491861e6b9a653d, 0x9a1019d7ab2c3fc2, 0x3668d42d06fe13d7,
        0xdcc1fbb25606a6d0, 0x969490dd795a1c22, 0x3549b1a1bc6dd2ef, 0xc94f5e23a0ed770e,
        0xb9f6686b5b39fdcb, 0xc4d4f4a6efeae00d, 0xe732851a1fff2204, 0x94aad6de5eb869f9,
        0x3f8ff2ae07206e7f, 0xfe38a9813b62d03a, 0xa7a1ad7a8bee2466, 0x7b6056c8dde882b6,
        0x302a1e286fc58ca7, 0x8da0fa457a259bc7, 0xb3302b64e074415b, 0x5402ae7eff8b635f,
        0x08f8050c9cafc94b, 0xae468bf98a3059ce, 0x88c355cca98dc58f, 0xb10e6d67c7963480,
        0xbad70de7e1aa3cf3, 0xbfb4a26e320262bb, 0xcb711820870f02d5, 0xce12b7a954a75c9d,
        0x563ce87dd8691684, 0x9f73b65e7884618a, 0x2b1e74b06cba0b42, 0x47cec1ea605b2df1,
        0x1c698312f735ac76, 0x5fdbcefed9b76b2c, 0x831a354c8fb1cdfc, 0x820516c312c0791f,
        0xb74ca762aeadabf0, 0xfc06ef821c80a5e1, 0x5723cbf24518a267, 0x9d4df05d5f661451,
        0x588627742dfd40bf, 0xda8331b73f3d39a0, 0x17b0e392d109a405, 0xf965400bcf28fba9,
        0x7c3dbf4229a2a925, 0x023e460327e275db, 0x6cd0b55a0ce126b3, 0xe62da695828e96e7,
        0x42ad6e63b3f373b9, 0xe50cc319381d57df, 0xc5cbd729729b54ee, 0x46d1e265fd2a9912,
        0x6428b056904eeff8, 0x8be23040131e04b7, 0x6709d5da2add2ec0, 0x075de98af44a2b93,
        0x8447dcc67bfbe66f, 0x6616f655b7ac9a23, 0xd607b8bded4b1a40, 0x0563af89d3a85e48,
        0x3db1b4ad20c21ba4, 0x11f22997b8323b75, 0x292032b34b587e99, 0x7f1cdace9331681d,
        0x8e819fc9c0b65aff, 0xa1e3677fe2d5bb16, 0xcd33d225ee349da5, 0xd9a2543b85aef898,
        0x795e10cbfa0af76d, 0x25a4bbb9992e5d79, 0x78413344677b438e, 0xf0826688cef68601,
        0xd27b34bba392f0eb, 0x551d8df162fad7bc, 0x1e57c511d0d7d9ad, 0xdeffbdb171e4d30b,
        0xf4feea8e802f6caa, 0xa480c8f6317de55e, 0xa0fc44f07fa40ff5, 0x95b5f551c3c9dd1a,
        0x22f952336d6476ea, 0x0000000000000000, 0xa6be8ef5169f9085, 0xcc2cf1aa73452946,
        0x2e7ddb39bf12550a, 0xd526dd3157d8db78, 0x486b2d6c08becf29, 0x9b0f3a58365d8b21,
        0xac78cdfaadd22c15, 0xbc95c7e28891a383, 0x6a927f5f65dab9c3, 0xc3891d2c1ba0cb9e,
        0xeaa92f9f50f8b507, 0xcf0d9426c9d6e87e, 0xca6e3baf1a7eb636, 0xab25247059980786,
        0x69b31ad3df4978fb, 0xe2512a93cc577c4c, 0xff278a0ea61364d9, 0x71a615c766a53e26,
        0x89dc764334fc716c, 0xf87a638452594f4a, 0xf2bc208be914f3da, 0x8766b94ac1682757,
        0xbbc82e687cdb8810, 0x626a7a53f9757088, 0xa2c202f358467a2e, 0x4d0882e5db169161,
        0x09e7268301de7da8, 0xe897699c771ac0dc, 0xc8507dac3d9cc3ed, 0xc0a878a0a1330aa6,
        0x978bb352e42ba8c1, 0xe9884a13ea6b743f, 0x279afdbabecc28a2, 0x047c8c064ed9eaab,
        0x507e2278b15289f4, 0x599904fbb08cf45c, 0xbd8ae46d15e01760, 0x31353da7f2b43844,
        0x8558ff49e68a528c, 0x76fbfc4d92ef15b5, 0x3456922e211c660c, 0x86799ac55c1993b4,
        0x3e90d1219a51da9c, 0x2d5cbeb505819432, 0x982e5fd48cce4a19, 0xdb9c1238a24c8d43,
        0xd439febecaa96f9b, 0x418c0bef0960b281, 0x158ea591f6ebd1de, 0x1f48e69e4da66d4e,
        0x8afd13cf8e6fb054, 0xf5e1c9011d5ed849, 0xe34e091c5126c8af, 0xad67ee7530a398f6,
        0x43b24dec2e82c75a, 0x75da99c1287cd48d, 0x92e81cdb3783f689, 0xa3dd217cc537cecd,
        0x60543c50de970553, 0x93f73f54aaf2426a, 0xa91b62737e7a725d, 0xf19d4507538732e2,
        0x77e4dfc20f9ea156, 0x7d229ccdb4d31dc6, 0x1b346a98037f87e5, 0xedf4c615a4b29e94,
        0x4093286094110662, 0xb0114ee85ae78063, 0x6ff1d0d6b672e78b, 0x6dcf96d591909250,
        0xdfe09e3eec9567e8, 0x3214582b4827f97c, 0xb46dc2ee143e6ac8, 0xf6c0ac8da7cd1971,
        0xebb60c10cd8901e4, 0xf7df8f023abcad92, 0x9c52d3d2c217a0b2, 0x6b8d5cd0f8ab0d20,
        0x3777f7a29b8fa734, 0x011f238f9d71b4e3, 0xc1b75b2f3c42be45, 0x5de588fdfe551ef7,
        0x6eeef3592b035368, 0xaa3a07ffc4e9b365, 0xecebe59a39c32a77, 0x5ba742f8976e8187,
        0x4b4a48e0b22d0e11, 0xddded83dcb771233, 0xa59feb79ac0c51bd, 0xc7f5912a55792135,
    },
    {
        0x6d6ae04668a9b08a, 0x3ab3f04b0be8c743, 0xe51e166b54b3c908, 0xbe90a9eb35c2f139,
        0xb2c7066637f2bec1, 0xaa6945613392202c, 0x9a28c36f3b5201eb, 0xddce5a93ab536994,
        0x0e34133ef6382827, 0x52a02ba1ec55048b, 0xa2f88f97c4b2a177, 0x8640e513ca2251a5,
        0xcdf1d36258137622, 0xfe6cb708dedf8ddb, 0x8a174a9ec8121e5d, 0x679896036b81560e,
        0x59ed033395795fee, 0x1dd778ab8b74edaf, 0xee533ef92d9f926d, 0x2a8c79baf8a8d8f5,
        0x6bcf398e69b119f6, 0xe20491742fafdd95, 0x276488e0809c2aec, 0xea955b82d88f5cce,
        0x7102c63a99d9e0c4, 0xf9763017a5c39946, 0x429fa2501f151b3d, 0x4659c72bea05d59e,
        0x984b7fdccf5a6634, 0xf742232953fbb161, 0x3041860e08c021c7, 0x747bfd9616cd9386,
        0x4bb1367192312787, 0x1b72a1638a6c44d3, 0x4a0e68a6e8359a66, 0x169a5039f258b6ca,
        0xb98a2ef44edee5a4, 0xd9083fe85e43a737, 0x967f6ce239624e13, 0x8874f62d3c1a7982,
        0x3c1629830af06e3f, 0x9165ebfd427e5a8e, 0xb5dd81794ceeaa5c, 0x0de8f15a7834f219,
        0x70bd98ede3dd5d25, 0xaccc9ca9328a8950, 0x56664eda1945ca28, 0x221db34c0f8859ae,
        0x26dbd637fa98970d, 0x1acdffb4f068f932, 0x4585254f64090fa0, 0x72de245e17d53afa,
        0x1546b25d7c546cf4, 0x207e0ffffb803e71, 0xfaaad2732bcf4378, 0xb462dfae36ea17bd,
        0xcf926fd1ac1b11fd, 0xe0672dc7dba7ba4a, 0xd3fa49ad5d6b41b3, 0x8ba81449b216a3bc,
        0x14f9ec8a0650d115, 0x40fc1ee3eb1d7ce2, 0x23a2ed9b758ce44f, 0x782c521b14fddc7e,
        0x1c68267cf170504e, 0xbcf31558c1ca96e6, 0xa781b43b4ba6d235, 0xf6fd7dfe29ff0c80,
        0xb0a4bad5c3fad91e, 0xd199f51ea963266c, 0x414340349119c103, 0x5405f269ed4dadf7,
        0xabd61bb649969dcd, 0x6813dbeae7bdc3c8, 0x65fb2ab09f8931d1, 0xf1e7fae152e3181d,
        0xc1a67cef5a2339da, 0x7a4feea8e0f5bba1, 0x1e0b9acf05783791, 0x5b8ebf8061713831,
        0x80e53cdbcb3af8d9, 0x7e898bd315e57502, 0xc6bcfbf0213f2d47, 0x95a38e86b76e942d,
        0x092e94218d243cba, 0x8339debf453622e7, 0xb11be402b9fe64ff, 0x57d9100d634177c9,
        0xcc4e8db52217cbc3, 0x3b0cae9c71ec7aa2, 0xfb158ca451cbfe99, 0x2b33276d82ac6514,
        0x01bf5ed77a04bde1, 0xc5601994af33f779, 0x75c4a3416cc92e67, 0xf3844652a6eb7fc2,
        0x3487e375fdd0ef64, 0x18ae430704609eed, 0x4d14efb993298efb, 0x815a620cb13e4538,
        0x125c354207487869, 0x9eeea614ce42cf48, 0xce2d3106d61fac1c, 0xbbe99247bad6827b,
        0x071a871f7b1c149d, 0x2e4a1cc10db81656, 0x77a71ff298c149b8, 0x06a5d9c80118a97c,
        0xad73c27e488e34b1, 0x443a7b981e0db241, 0xe3bbcfa355ab6074, 0x0af276450328e684,
        0x73617a896dd1871b, 0x58525de4ef7de20f, 0xb7be3dcab8e6cd83, 0x19111dd07e64230c,
        0x842359a03e2a367a, 0x103f89f1f3401fb6, 0xdc710444d157d475, 0xb835702334da5845,
        0x4320fc876511a6dc, 0xd026abc9d3679b8d, 0x17250eee885c0b2b, 0x90dab52a387ae76f,
        0x31fed8d972c49c26, 0x89cba8fa461ec463, 0x2ff5421677bcabb7, 0x396f122f85e41d7d,
        0xa09b332430bac6a8, 0xc888e8ced7070560, 0xaeaf201ac682ee8f, 0x1180d7268944a257,
        0xf058a43628e7a5fc, 0xbd4c4b8fbbce2b07, 0xa1246df34abe7b49, 0x7d5569b79be9af3c,
        0xa9b5a705bd9efa12, 0xdb6b835baa4bc0e8, 0x05793bac8f147342, 0x21c1512881848390,
        0xfdb0556c50d357e5, 0x613d4fcb6a99ff72, 0x03dce2648e0cda3e, 0xe949b9e6568386f0,
        0xfc0f0bbb2ad7ea04, 0x6a70675913b5a417, 0x7f36d5046fe1c8e3, 0x0c57af8d02304ff8,
        0x32223abdfcc84618, 0x0891caf6f720815b, 0xa63eeaec31a26fd4, 0x2507345374944d33,
        0x49d28ac266394058, 0xf5219f9aa7f3d6be, 0x2d96fea583b4cc68, 0x5a31e1571b7585d0,
        0x8ed12fe53d02d0fe, 0xdfade6205f5b0e4b, 0x4cabb16ee92d331a, 0x04c6657bf510cea3,
        0xd73c2cd6a87b8f10, 0xe1d87310a1a307ab, 0x6cd5be9112ad0d6b, 0x97c032354366f3f2,
        0xd4e0ceb22677552e, 0x0000000000000000, 0x29509bde76a402cb, 0xc27a9e8bd42fe3e4,
        0x5ef7842cee654b73, 0xaf107ecdbc86536e, 0x3fcacbe784fcb401, 0xd55f90655c73e8cf,
        0xe6c2f40fdabf1336, 0xe8f6e7312c873b11, 0xeb2a0555a28be12f, 0xe4a148bc2eb774e9,
        0x9b979db84156bc0a, 0x6eb60222e6a56ab4, 0x87ffbbc4b026ec44, 0xc703a5275b3b90a6,
        0x47e699fc9001687f, 0x9c8d1aa73a4aa897, 0x7cea3760e1ed12dd, 0x4ec80ddd1d2554c5,
        0x13e36b957d4cc588, 0x5d2b66486069914d, 0x92b90999cc7280b0, 0x517cc9c56259deb5,
        0xc937b619ad03b881, 0xec30824ad997f5b2, 0xa45d565fc5aa080b, 0xd6837201d27f32f1,
        0x635ef3789e9198ad, 0x531f75769651b96a, 0x4f77530a6721e924, 0x486dd4151c3dfdb9,
        0x5f48dafb9461f692, 0x375b011173dc355a, 0x3da9775470f4d3de, 0x8d0dcd81b30e0ac0,
        0x36e45fc609d888bb, 0x55baacbe97491016, 0x8cb29356c90ab721, 0x76184125e2c5f459,
        0x99f4210bb55edbd5, 0x6f095cf59ca1d755, 0x9f51f8c3b44672a9, 0x3538bda287d45285,
        0x50c39712185d6354, 0xf23b1885dcefc223, 0x79930ccc6ef9619f, 0xed8fdc9da3934853,
        0xcb540aaa590bdf5e, 0x5c94389f1a6d2cac, 0xe77daad8a0bbaed7, 0x28efc5090ca0bf2a,
        0xbf2ff73c4fc64cd8, 0xb37858b14df60320, 0xf8c96ec0dfc724a7, 0x828680683f329f06,
        0x941cd051cd6a29cc, 0xc3c5c05cae2b5e05, 0xb601631dc2e27062, 0xc01922382027843b,
        0x24b86a840e90f0d2, 0xd245177a276ffc52, 0x0f8b4de98c3c95c6, 0x3e759530fef809e0,
        0x0b4d2892792c5b65, 0xc4df4743d5374a98, 0xa5e20888bfaeb5ea, 0xba56cc90c0d23f9a,
        0x38d04cf8ffe0a09c, 0x62e1adafe495254c, 0x0263bcb3f40867df, 0xcaeb547d230f62bf,
        0x6082111c109d4293, 0xdad4dd8cd04f7d09, 0xefec602e579b2f8c, 0x1fb4c4187f7c8a70,
        0xffd3e9dfa4db303a, 0x7bf0b07f9af10640, 0xf49ec14dddf76b5f, 0x8f6e713247066d1f,
        0x339d646a86ccfbf9, 0x64447467e58d8c30, 0x2c29a072f9b07189, 0xd8b7613f24471ad6,
        0x6627c8d41185ebef, 0xa347d140beb61c96, 0xde12b8f7255fb3aa, 0x9d324470404e1576,
        0x9306574eb6763d51, 0xa80af9d2c79a47f3, 0x859c0777442e8b9b, 0x69ac853d9db97e29,
    },
    {
        0xc3407dfc2de6377e, 0x5b9e93eea4256f77, 0xadb58fdd50c845e0, 0x5219ff11a75bed86,
        0x356b61cfd90b1de9, 0xfb8f406e25abe037, 0x7a5a0231c0f60796, 0x9d3cd216e1f5020b,
        0x0c6550fb6b48d8f3, 0xf57508c427ff1c62, 0x4ad35ffa71cb407d, 0x6290a2da1666aa6d,
        0xe284ec2349355f9f, 0xb3c307c53d7c84ec, 0x05e23c0468365a02, 0x190bac4d6c9ebfa8,
        0x94bbbee9e28b80fa, 0xa34fc777529cb9b5, 0xcc7b39f095bcd978, 0x2426addb0ce532e3,
        0x7e79329312ce4fc7, 0xab09a72eebec2917, 0xf8d15499f6b9d6c2, 0x1a55b8babf8c895d,
        0xdb8add17fb769a85, 0xb57f2f368658e81b, 0x8acd36f18f3f41f6, 0x5ce3b7bba50f11d3,
        0x114dcc14d5ee2f0a, 0xb91a7fcded1030e8, 0x81d5425fe55de7a1, 0xb6213bc1554adeee,
        0x80144ef95f53f5f2, 0x1e7688186db4c10c, 0x3b912965db5fe1bc, 0xc281715a97e8252d,
        0x54a5d7e21c7f8171, 0x4b12535ccbc5522e, 0x1d289cefbea6f7f9, 0x6ef5f2217d2e729e,
        0xe6a7dc819b0d17ce, 0x1b94b41c05829b0e, 0x33d7493c622f711e, 0xdcf7f942fa5ce421,
        0x600fba8b7f7a8ecb, 0x46b60f011a83988e, 0x235b898e0dcf4c47, 0x957ab24f588592a9,
        0x4354330572b5c28c, 0xa5f3ef84e9b8d542, 0x8c711e02341b2d01, 0x0b1874ae6a62a657,
        0x1213d8e306fc19ff, 0xfe6d7c6a4d9dba35, 0x65ed868f174cd4c9, 0x88522ea0e6236550,
        0x899322065c2d7703, 0xc01e690bfef4018b, 0x915982ed8abddaf8, 0xbe675b98ec3a4e4c,
        0xa996bf7f82f00db1, 0xe1daf8d49a27696a, 0x2effd5d3dc8986e7, 0xd153a51f2b1a2e81,
        0x18caa0ebd690adfb, 0x390e3134b243c51a, 0x2778b92cdff70416, 0x029f1851691c24a6,
        0x5e7cafeacc133575, 0xfa4e4cc89fa5f264, 0x5a5f9f481e2b7d24, 0x484c47ab18d764db,
        0x400a27f2a1a7f479, 0xaeeb9b2a83da7315, 0x721c626879869734, 0x042330a2d2384851,
        0x85f672fd3765aff0, 0xba446b3a3e02061d, 0x73dd6ecec3888567, 0xffac70ccf793a866,
        0xdfa9edb5294ed2d4, 0x6c6aea7014325638, 0x834a5a0e8c41c307, 0xcdba35562fb2cb2b,
        0x0ad97808d06cb404, 0x0f3b440cb85aee06, 0xe5f9c876481f213b, 0x98deee1289c35809,
        0x59018bbfcd394bd1, 0xe01bf47220297b39, 0xde68e1139340c087, 0x9fa3ca4788e926ad,
        0xbb85679c840c144e, 0x53d8f3b71d55ffd5, 0x0da45c5dd146caa0, 0x6f34fe87c72060cd,
        0x57fbc315cf6db784, 0xcee421a1fca0fdde, 0x3d2d0196607b8d4b, 0x642c8a29ad42c69a,
        0x14aff010bdd87508, 0xac74837beac657b3, 0x3216459ad821634d, 0x3fb219c70967a9ed,
        0x06bc28f3bb246cf7, 0xf2082c9126d562c6, 0x66b39278c45ee23c, 0xbd394f6f3f2878b9,
        0xfd33689d9e8f8cc0, 0x37f4799eb017394f, 0x108cc0b26fe03d59, 0xda4bd1b1417888d6,
        0xb09d1332ee6eb219, 0x2f3ed975668794b4, 0x58c0871977375982, 0x7561463d78ace990,
        0x09876cff037e82f1, 0x7fb83e35a8c05d94, 0x26b9b58a65f91645, 0xef20b07e9873953f,
        0x3148516d0b3355b8, 0x41cb2b541ba9e62a, 0x790416c613e43163, 0xa011d380818e8f40,
        0x3a5025c36151f3ef, 0xd57095bdf92266d0, 0x498d4b0da2d97688, 0x8b0c3a57353153a5,
        0x21c491df64d368e1, 0x8f2f0af5e7091bf4, 0x2da1c1240f9bb012, 0xc43d59a92ccc49da,
        0xbfa6573e56345c1f, 0x828b56a8364fd154, 0x9a41f643e0df7caf, 0xbcf843c985266aea,
        0x2b1de9d7b4bfdce5, 0x20059d79dedd7ab2, 0x6dabe6d6ae3c446b, 0x45e81bf6c991ae7b,
        0x6351ae7cac68b83e, 0xa432e32253b6c711, 0xd092a9b991143cd2, 0xcac711032e98b58f,
        0xd8d4c9e02864ac70, 0xc5fc550f96c25b89, 0xd7ef8dec903e4276, 0x67729ede7e50f06f,
        0xeac28c7af045cf3d, 0xb15c1f945460a04a, 0x9cfddeb05bfb1058, 0x93c69abce3a1fe5e,
        0xeb0380dc4a4bdd6e, 0xd20db1e8f8081874, 0x229a8528b7c15e14, 0x44291750739fbc28,
        0xd3ccbd4e42060a27, 0xf62b1c33f4ed2a97, 0x86a8660ae4779905, 0xd62e814a2a305025,
        0x477703a7a08d8add, 0x7b9b0e977af815c5, 0x78c51a60a9ea2330, 0xa6adfb733aaae3b7,
        0x97e5aa1e3199b60f, 0x0000000000000000, 0xf4b404629df10e31, 0x5564db44a6719322,
        0x9207961a59afec0d, 0x9624a6b88b97a45c, 0x363575380a192b1c, 0x2c60cd82b595a241,
        0x7d272664c1dc7932, 0x7142769faa94a1c1, 0xa1d0df263b809d13, 0x1630e841d4c451ae,
        0xc1df65ad44fa13d8, 0x13d2d445bcf20bac, 0xd915c546926abe23, 0x38cf3d92084dd749,
        0xe766d0272103059d, 0xc7634d5effde7f2f, 0x077d2455012a7ea4, 0xedbfa82ff16fb199,
        0xaf2a978c39d46146, 0x42953fa3c8bbd0df, 0xcb061da59496a7dc, 0x25e7a17db6eb20b0,
        0x34aa6d6963050fba, 0xa76cf7d580a4f1e4, 0xf7ea10954ee338c4, 0xfcf2643b24819e93,
        0xcf252d0746aeef8d, 0x4ef06f58a3f3082c, 0x563acfb37563a5d7, 0x5086e740ce47c920,
        0x2982f186dda3f843, 0x87696aac5e798b56, 0x5d22bb1d1f010380, 0x035e14f7d31236f5,
        0x3cec0d30da759f18, 0xf3c920379cdb7095, 0xb8db736b571e22bb, 0xdd36f5e44052f672,
        0xaac8ab8851e23b44, 0xa857b3d938fe1fe2, 0x17f1e4e76eca43fd, 0xec7ea4894b61a3ca,
        0x9e62c6e132e734fe, 0xd4b1991b432c7483, 0x6ad6c283af163acf, 0x1ce9904904a8e5aa,
        0x5fbda34c761d2726, 0xf910583f4cb7c491, 0xc6a241f845d06d7c, 0x4f3163fe19fd1a7f,
        0xe99c988d2357f9c8, 0x8eee06535d0709a7, 0x0efa48aa0254fc55, 0xb4be23903c56fa48,
        0x763f52caabbedf65, 0xeee1bcd8227d876c, 0xe345e085f33b4dcc, 0x3e731561b369bbbe,
        0x2843fd2067adea10, 0x2adce5710eb1ceb6, 0xb7e03767ef44ccbd, 0x8db012a48e153f52,
        0x61ceb62dc5749c98, 0xe85d942b9959eb9b, 0x4c6f7709caef2c8a, 0x84377e5b8d6bbda3,
        0x30895dcbb13d47eb, 0x74a04a9bc2a2fbc3, 0x6b17ce251518289c, 0xe438c4d0f2113368,
        0x1fb784bed7bad35f, 0x9b80fae55ad16efc, 0x77fe5e6c11b0cd36, 0xc858095247849129,
        0x08466059b97090a2, 0x01c10ca6ba0e1253, 0x6988d6747c040c3a, 0x6849dad2c60a1e69,
        0x5147ebe67449db73, 0xc99905f4fd8a837a, 0x991fe2b433cd4a5a, 0xf09734c04fc94660,
        0xa28ecbd1e892abe6, 0xf1563866f5c75433, 0x4dae7baf70e13ed9, 0x7ce62ac27bd26b61,
        0x70837a39109ab392, 0x90988e4b30b3c8ab, 0xb2020b63877296bf, 0x156efcb607d6675b,
    },
    {
        0xe63f55ce97c331d0, 0x25b506b0015bba16, 0xc8706e29e6ad9ba8, 0x5b43d3775d521f6a,
        0x0bfa3d577035106e, 0xab95fc172afb0e66, 0xf64b63979e7a3276, 0xf58b4562649dad4b,
        0x48f7c3dbae0c83f1, 0xff31916642f5c8c5, 0xcbb048dc1c4a0495, 0x66b8f83cdf622989,
        0x35c130e908e2b9b0, 0x7c761a61f0b34fa1, 0x3601161cf205268d, 0x9e54ccfe2219b7d6,
        0x8b7d90a538940837, 0x9cd403588ea35d0b, 0xbc3c6fea9ccc5b5a, 0xe5ff733b6d24aeed,
        0xceed22de0f7eb8d2, 0xec8581cab1ab545e, 0xb96105e88ff8e71d, 0x8ca03501871a5ead,
        0x76ccce65d6db2a2f, 0x5883f582a7b58057, 0x3f7be4ed2e8adc3e, 0x0fe7be06355cd9c9,
        0xee054e6c1d11be83, 0x1074365909b903a6, 0x5dde9f80b4813c10, 0x4a770c7d02b6692c,
        0x5379c8d5d7809039, 0xb4067448161ed409, 0x5f5e5026183bd6cd, 0xe898029bf4c29df9,
        0x7fb63c940a54d09c, 0xc5171f897f4ba8bc, 0xa6f28db7b31d3d72, 0x2e4f3be7716eaa78,
        0x0d6771a099e63314, 0x82076254e41bf284, 0x2f0fd2b42733df98, 0x5c9e76d3e2dc49f0,
        0x7aeb569619606cdb, 0x83478b07b2468764, 0xcfadcb8d5923cd32, 0x85dac7f05b95a41e,
        0xb5469d1b4043a1e9, 0xb821ecbbd9a592fd, 0x1b8e0b0e798c13c8, 0x62a57b6d9a0be02e,
        0xfcf1b793b81257f8, 0x9d94ea0bd8fe28eb, 0x4cea408aeb654a56, 0x23284a47e888996c,
        0x2d8f1d128b893545, 0xf4cbac3132c0d8ab, 0xbd7c86b9ca912eba, 0x3a268eef3dbe6079,
        0xf0d62f6077a9110c, 0x2735c916ade150cb, 0x89fd5f03942ee2ea, 0x1acee25d2fd16628,
        0x90f39bab41181bff, 0x430dfe8cde39939f, 0xf70b8ac4c8274796, 0x1c53aeaac6024552,
        0x13b410acf35e9c9b, 0xa532ab4249faa24f, 0x2b1251e5625a163f, 0xd7e3e676da4841c7,
        0xa7b264e4e5404892, 0xda8497d643ae72d3, 0x861ae105a1723b23, 0x38a6414991048aa4,
        0x6578dec92585b6b4, 0x0280cfa6acbaeadd, 0x88bdb650c273970a, 0x9333bd5ebbff84c2,
        0x4e6a8f2c47dfa08b, 0x321c954db76cef2a, 0x418d312a72837942, 0xb29b38bfffcdf773,
        0x6c022c38f90a4c07, 0x5a033a240b0f6a8a, 0x1f93885f3ce5da6f, 0xc38a537e96988bc6,
        0x39e6a81ac759ff44, 0x29929e43cee0fce2, 0x40cdd87924de0ca2, 0xe9d8ebc8a29fe819,
        0x0c2798f3cfbb46f4, 0x55e484223e53b343, 0x4650948ecd0d2fd8, 0x20e86cb2126f0651,
        0x6d42c56baf5739e7, 0xa06fc1405ace1e08, 0x7babbfc54f3d193b, 0x424d17df8864e67f,
        0xd8045870ef14980e, 0xc6d7397c85ac3781, 0x21a885e1443273b1, 0x67f8116f893f5c69,
        0x24f5efe35706cff6, 0xd56329d076f2ab1a, 0x5e1eb9754e66a32d, 0x28d2771098bd8902,
        0x8f6013f47dfdc190, 0x17a993fdb637553c, 0xe0a219397e1012aa, 0x786b9930b5da8606,
        0x6e82e39e55b0a6da, 0x875a0856f72f4ec3, 0x3741ff4fa458536d, 0xac4859b3957558fc,
        0x7ef6d5c75c09a57c, 0xc04a758b6c7f14fb, 0xf9acdd91ab26ebbf, 0x7391a467c5ef9668,
        0x335c7c1ee1319aca, 0xa91533b18641e4bb, 0xe4bf9a683b79db0d, 0x8e20faa72ba0b470,
        0x51f907737b3a7ae4, 0x2268a314bed5ec8c, 0xd944b123b949edee, 0x31dcb3b84d8b7017,
        0xd3fe65279f218860, 0x097af2f1dc8ffab3, 0x9b09a6fc312d0b91, 0xcc6ded78a3c4520f,
        0x3481d9ba5ebfcc50, 0x4f2a667f1182d56b, 0xdfd9fdd4509ace94, 0x26752045fbbc252b,
        0xbffc491f662bc467, 0xdd593272fc202449, 0x3cbbc218d46d4303, 0x91b372f817456e1f,
        0x681faf69bc6385a0, 0xb686bbeebaa43ed4, 0x1469b5084cd0ca01, 0x98c98009cbca94ac,
        0x6438379a73d8c354, 0xc2caba2dc0c5fe26, 0x3e3b0dbe78d7a9de, 0x50b9ee202d670f04,
        0x4590b27b37eab0e5, 0x6025b4cb36b10af3, 0xfb2c1237079c0162, 0xa12f28130c936be8,
        0x4b37e52e54eb1ccc, 0x083a1ba28ad28f53, 0xc10a9cd83a22611b, 0x9f1425ad7444c236,
        0x069d4cf7e9d3237a, 0xedc56899e7f621be, 0x778c273680865fcf, 0x309c5aeb1bd605f7,
        0x8de0dc52d1472b4d, 0xf8ec34c2fd7b9e5f, 0xea18cd3d58787724, 0xaad515447ca67b86,
        0x9989695a9d97e14c, 0x0000000000000000, 0xf196c63321f464ec, 0x71116bc169557cb5,
        0xaf887f466f92c7c1, 0x972e3e0ffe964d65, 0x190ec4a8d536f915, 0x95aef1a9522ca7b8,
        0xdc19db21aa7d51a9, 0x94ee18fa0471d258, 0x8087adf248a11859, 0xc457f6da2916dd5c,
        0xfa6cfb6451c17482, 0xf256e0c6db13fbd1, 0x6a9f60cf10d96f7d, 0x4daaa9d9bd383fb6,
        0x03c026f5fae79f3d, 0xde99148706c7bb74, 0x2a52b8b6340763df, 0x6fc20acd03edd33a,
        0xd423c08320afdefa, 0xbbe1ca4e23420dc0, 0x966ed75ca8cb3885, 0xeb58246e0e2502c4,
        0x055d6a021334bc47, 0xa47242111fa7d7af, 0xe3623fcc84f78d97, 0x81c744a11efc6db9,
        0xaec8961539cfb221, 0xf31609958d4e8e31, 0x63e5923ecc5695ce, 0x47107ddd9b505a38,
        0xa3afe7b5a0298135, 0x792b7063e387f3e6, 0x0140e953565d75e0, 0x12f4f9ffa503e97b,
        0x750ce8902c3cb512, 0xdbc47e8515f30733, 0x1ed3610c6ab8af8f, 0x5239218681dde5d9,
        0xe222d69fd2aaf877, 0xfe71783514a8bd25, 0xcaf0a18f4a177175, 0x61655d9860ec7f13,
        0xe77fbc9dc19e4430, 0x2ccff441ddd440a5, 0x16e97aaee06a20dc, 0xa855dae2d01c915b,
        0x1d1347f9905f30b2, 0xb7c652bdecf94b34, 0xd03e43d265c6175d, 0xfdb15ec0ee4f2218,
        0x57644b8492e9599e, 0x07dda5a4bf8e569a, 0x54a46d71680ec6a3, 0x5624a2d7c4b42c7e,
        0xbebca04c3076b187, 0x7d36f332a6ee3a41, 0x3b6667bc6be31599, 0x695f463aea3ef040,
        0xad08b0e0c3282d1c, 0xb15b1e4a052a684e, 0x44d05b2861b7c505, 0x15295c5b1a8dbfe1,
        0x744c01c37a61c0f2, 0x59c31cd1f1e8f5b7, 0xef45a73f4b4ccb63, 0x6bdf899c46841a9d,
        0x3dfb2b4b823036e3, 0xa2ef0ee6f674f4d5, 0x184e2dfb836b8cf5, 0x1134df0a5fe47646,
        0xbaa1231d751f7820, 0xd17eaa81339b62bd, 0xb01bf71953771dae, 0x849a2ea30dc8d1fe,
        0x705182923f080955, 0x0ea757556301ac29, 0x041d83514569c9a7, 0x0abad4042668658e,
        0x49b72a88f851f611, 0x8a3d79f66ec97dd7, 0xcd2d042bf59927ef, 0xc930877ab0f0ee48,
        0x9273540deda2f122, 0xc797d02fd3f14261, 0xe1e2f06a284d674a, 0xd2be8c74c97cfd80,
        0x9a494faf67707e71, 0xb3dbd1eca9908293, 0x72d14d3493b2e388, 0xd6a30f258c153427,
    },
};


}
