import random
import subprocess
import multiprocessing

from deap import base
from deap import creator
from deap import tools
from deap import algorithms
from scoop import futures
import numpy
import pickle

def eaSimpleGIO(population, toolbox, cxpb, mutpb, ngen, stats=None,
             verbose=__debug__):
    halloffame = [tools.HallOfFame(maxsize=1) for _ in range(ngen + 2) ]
    logbook = tools.Logbook()
    logbook.header = ['gen', 'nevals'] + (stats.fields if stats else [])

    best = None
    seed = random.getrandbits(10)

    # Evaluate the individuals with an invalid fitness
    invalid_ind_enhance = [(ind, best, seed) for ind in population]
    fitnesses = toolbox.map(toolbox.evaluate, invalid_ind_enhance)
    for ind, fit in zip(population, fitnesses):
        ind.fitness.values = fit

    halloffame[0].update(population)

    record = stats.compile(population) if stats else {}
    logbook.record(gen=0, nevals=len(population), **record)
    if verbose:
        print(logbook.stream)

    # Begin the generational process
    for gen in range(1, ngen + 1):
        best = halloffame[gen-1][0]
        # Select the next generation individuals
        offspring = toolbox.select(population, len(population))

        # Vary the pool of individuals
        offspring = algorithms.varAnd(offspring, toolbox, cxpb, mutpb)

        # Evaluate the individuals with an invalid fitness
        invalid_ind_enhance = [(ind, best, seed) for ind in offspring]
        fitnesses = toolbox.map(toolbox.evaluate, invalid_ind_enhance)
        for ind, fit in zip(offspring, fitnesses):
            ind.fitness.values = fit

        # Update the hall of fame with the generated individuals
        halloffame[gen].update(offspring)

        # Replace the current population by the offspring
        population[:] = offspring

        # Append the current generation statistics to the logbook
        record = stats.compile(population) if stats else {}
        logbook.record(gen=gen, nevals=len(offspring), **record)
        if verbose:
            print(logbook.stream)
        print(f'\n\n\nFAME ======== {halloffame[gen]}\n\n\n')

        cp = dict(population=population, generation=gen, halloffame=halloffame,logbook=logbook, rndstate=random.getstate())
        with open(f'evolution/checkpoint/checkpoint_name_{gen}.pkl', "wb") as cp_file:
            pickle.dump(cp, cp_file)

    return population, logbook


def evaluateInd(tt):
    t1 = tt[0]
    t2 = tt[1]
    random.seed(tt[2])

    if t2 is None:
        t2 = t1

    avrg = 0
    save = []

    for i in range(5):
        SZ = [32, 40, 48, 56, 64]
        width = height = SZ[i]
        seed = random.randint(a=1, b=99999999)

        args = './halite --no-logs --no-replay --no-timeout '
        args = args + f'--seed {seed} '
        args = args + f'--width {width} --height {height} '

        args = args + f'"./MyBot '
        args = args + f'{seed} ' + ' '.join([str(x) for x in t1]) + '" '


        args = args + f'"./MyBot '
        args = args + f'{seed} ' + ' '.join([str(x) for x in t2]) + '"'

        save.append(args)

        st1 = "Player 0, 'MyCppBot', was rank 1 with "
        st2 = "Player 0, 'MyCppBot', was rank 1 with "

        out = subprocess.getoutput(args).split('\n')
        line = [line for line in out if st1 in line or st2 in line]
        if len(line) == 0:
            avrg += 0
        else:
            line = line[0]
            if st1 in line:
                st = st1
            else:
                st = st2
            avrg += int(line[line.find(st)+len(st):].split()[0])

    avrg /= 5.0
    with open(f'evolution/{avrg}_random_res_{random.randint(1,100000)}.log', "w") as f:
        f.write(f'{tt[0]} = {tt[1]} = {save} = {avrg}')
    return (avrg, )


if __name__ == '__main__':


    creator.create("FitnessMax", base.Fitness, weights=(1.0,))
    creator.create("Individual", list, fitness=creator.FitnessMax)

    IND_SIZE=6

    toolbox = base.Toolbox()



    pool = multiprocessing.Pool()
    toolbox.register("map", pool.map)

    toolbox.register("attr_float", random.random)
    toolbox.register("individual", tools.initRepeat, creator.Individual,
                     toolbox.attr_float, n=IND_SIZE)
    toolbox.register("population", tools.initRepeat, list, toolbox.individual)



    toolbox.register("mate", tools.cxTwoPoint)
    toolbox.register("mutate", tools.mutGaussian, mu=0, sigma=1, indpb=0.2)
    toolbox.register("select", tools.selTournament, tournsize=3)
    toolbox.register("evaluate", evaluateInd)


    stats = tools.Statistics(lambda ind: ind.fitness.values)
    stats.register("Avg", numpy.mean)
    stats.register("Std", numpy.std)
    stats.register("Min", numpy.min)
    stats.register("Max", numpy.max)

    print(eaSimpleGIO(toolbox.population(n=10), toolbox, cxpb=0.5, mutpb=0.2, ngen=20, stats=stats, verbose=True))
