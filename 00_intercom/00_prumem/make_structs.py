import optparse as o
o = o.OptionParser()
opts, args = o.parse_args()

emsg = """/*
 * This file is auto-generated from {source_file}.  Any edits will be lost.
 */
"""

for infile in args:
    assert(infile.endswith('.in'))
    out_c, out_hp = [], []
    active = None
    out_hp.append(emsg.format(source_file=infile))
    out_c.append(emsg.format(source_file=infile))
    out_c.append('#pragma pack(push)\n#pragma pack(1)')
    for line in open(infile):
        line = line.strip('\n')
        w = line.split()
        cmd = None
        if len(w) > 0:
            cmd = w[0]
        if cmd == '.struct':
            assert(active is None)
            out_c.append('typedef struct {')
            out_hp.append('.struct %s' % w[1])
            active = w[-1]
        elif cmd in ['.u32', '.u16', '.u8']:
            if '[' in w[1]:
                i0, i1 = w[1].index('['), w[1].index(']')
                v, n = w[1][:i0], int(w[1][i0+1:i1])
                out_c.append('    %s_t %s[%i];' % (cmd.replace('.u', 'uint'), v, n))
                for i in range(n):
                    out_hp.append('    %s %s%i' % (cmd, v, i))
            else:
                out_c.append('    %s_t %s;' % (cmd.replace('.u', 'uint'), w[-1]))
                out_hp.append('    %s %s' % (cmd, w[1]))
        elif cmd == '.ends':
            out_c.append('} %s;' %active)
            active = None
            out_hp.append(cmd)
        else:
            out_c.append(line)
            out_hp.append(line)
    out_c.append('#pragma pack(pop)')
    for filename, lines in [(infile[:-3] + '.h', out_c),
                            (infile[:-3] + '.hp', out_hp)]:
        print 'Writing %s' % filename
        fout = open(filename, 'w')
        for line in lines:
            fout.write(line + '\n')
        fout.close()

