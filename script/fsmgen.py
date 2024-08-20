import sys


class State:
    def __init__(self, name, default_child, childs) -> None:
        self.name = name
        self.default_child = default_child
        self.childs = childs
    
    def print(self):
        print(self.name,':', self.default_child,':',self.childs)

state_list :list[State] = []

def parse(filename) :
    with open(filename, 'r') as file: 
        lines = file.readlines() 
        for line in lines:
            line = line.replace('\n','')
            if line.count('*') != 1: 
                print('line error: (', line, ') need one \'*\' means default state')
                continue
            if line.count('=') != 1:
                print('line error: ('+line+') need one \'=\'')
                continue
            line_split = line.replace(' ','').split('=')
            state = line_split[0]
            childs = line_split[1].split(',')
            for i, c in enumerate(childs):
                if c.count('*'):
                    childs[i] = c.replace('*','')
                    default_child = childs[i]
            state_list.append(State(state, default_child, childs))
            state_list[-1].print()


if __name__ == '__main__':
    print('parse: ',sys.argv[1])
    parse(sys.argv[1])